/*
g++ -std=c++17 -O3 -Iglad/include -ITinyPngOut/include main.cpp glad/src/glad.c TinyPngOut/src/TinyPngOut.cpp -lSOIL -lstdc++fs -lGL -lGLU -lglfw3 -lX11 -lXxf86vm -lXrandr -lpthread -lXi -ldl -lXinerama -lXcursor && ./a.out
*/
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <TinyPngOut.hpp>

#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <functional>
#include <numeric>
#include <chrono>
#include <thread>
#include <random>


struct Color {
	float r, g, b, a = 1.0f;
};


class Renderer {
	private: static std::string getFileContent(std::string filename) {
		std::ifstream file(filename);
		return std::string(std::istreambuf_iterator<char>(file),
			std::istreambuf_iterator<char>());
	}

	private: static void checkShader(int shader, const std::string& name) {
		int success;
		char infoLog[512];
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(shader, 512, NULL, infoLog);
			std::cout << name << " shader compilation failed:\n" << infoLog << "\n";
		}
	}

	private: static void checkProgram(int program) {
		int success;
		char infoLog[512];
		glGetProgramiv(program, GL_LINK_STATUS, &success);
		if (!success) {
			glGetProgramInfoLog(program, 512, NULL, infoLog);
			std::cout << "program linking failed:\n" << infoLog << std::endl;
		}
	}

	private: static int compileShader(const std::string& vertexFilename, const std::string& fragmentFilename) {
		std::string vertexShaderSourceStr = getFileContent(vertexFilename);
		std::string fragmentShaderSourceStr = getFileContent(fragmentFilename);
		const char* vertexShaderSource = vertexShaderSourceStr.c_str();
		const char* fragmentShaderSource = fragmentShaderSourceStr.c_str();

		// compila vertex shader
		int vertexShader = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
		glCompileShader(vertexShader);
		checkShader(vertexShader, "vertex");

		// compila fragment shader
		int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
		glCompileShader(fragmentShader);
		checkShader(fragmentShader, "fragment");

		// link shaders
		int shaderProgram = glCreateProgram();
		glAttachShader(shaderProgram, vertexShader);
		glAttachShader(shaderProgram, fragmentShader);
		glLinkProgram(shaderProgram);
		checkProgram(shaderProgram);
		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);

		return shaderProgram;
	}

	private: static void genVboVao(
			unsigned int shader,
			const std::vector<std::pair<std::string, int>>& attribs,
			unsigned int& vbo,
			unsigned int& vao) {
		glGenVertexArrays(1, &vao); // predisponimi un VAO e salva un identificatore in `vao_id`
		glGenBuffers(1, &vbo); // predisponimi un VBO e salva un identificatore in `vbo_id`

		glBindVertexArray(vao); // voglio usare il VAO all'id `vao_id`
		glBindBuffer(GL_ARRAY_BUFFER, vbo); // voglio usare il VBO all'id `vbo_id`


		int totalSize = 0;
		for (auto&& attrib : attribs) {
			totalSize += attrib.second;
		}

		int sizeSoFar = 0;
		for (auto&& attrib : attribs) {
			int location = glGetAttribLocation(shader, attrib.first.c_str());
			glVertexAttribPointer(location, attrib.second, GL_FLOAT, GL_FALSE, totalSize * sizeof(float), (void*)(sizeSoFar * sizeof(float)));
			glEnableVertexAttribArray(location);
			sizeSoFar += attrib.second;
		}
	}


	private: GLFWwindow* getWindow() {
		glfwInit();
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		#ifdef __APPLE__
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // fix compilation on OS X
		#endif

		// glfw window creation
		GLFWwindow* window = glfwCreateWindow(width, height, "LearnOpenGL", nullptr, nullptr);
		if (window == nullptr) {
			throw std::runtime_error("Failed to create GLFW window");
		}
		glfwMakeContextCurrent(window);

		// glad: load all OpenGL function pointers
		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
			throw std::runtime_error("Failed to initialize GLAD");
		}

		return window;
	}

	private: void swapBuffers() {
		GLenum glError = glGetError();
		if (glError != 0) {
			std::cout<<"glError: "<<glError<<"\n";
		}

		glfwSwapBuffers(window);
		glfwPollEvents();

		glClearColor(backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // also clear the depth buffer now!
	}

	private: void drawVertices() {
		glEnable(GL_DEPTH_TEST);
		glUseProgram(shader);
		glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLES, 0, nrVertices);
	}

	private: void drawLineVertices() {
		glDisable(GL_DEPTH_TEST); // no depth testing!
		glUseProgram(lineShader);
		glBindVertexArray(lineVao);
		glDrawArrays(GL_LINES, 0, nrLineVertices);
	}

	private: void saveScreenshot(int x, int y, unsigned int w, unsigned int h, const std::string& filename) {
		std::vector<uint8_t> pixels(3 * w * h);
		glReadPixels(x, y, w, h, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

		for(int line = 0; line != h/2; ++line) {
			std::swap_ranges(	pixels.begin() + 3 * w * line,
									pixels.begin() + 3 * w * (line+1),
									pixels.begin() + 3 * w * (h-line-1));
		}

		std::ofstream screenshotFile{filename, std::ios::binary};
		TinyPngOut{w, h, screenshotFile}.write(pixels.data(), w * h);
	}


	GLFWwindow* window;
	unsigned int shader, lineShader;
	unsigned int vbo, vao, lineVbo, lineVao;

	const unsigned int width, height;
	const float screenRatio;
	Color backgroundColor;
	size_t nrVertices, nrLineVertices;


	public: Renderer(unsigned int w, unsigned int h)
			: width{w}, height{h}, screenRatio{(float) w / h},
				nrVertices{0}, nrLineVertices{0} {

		window = getWindow();
		shader = compileShader("vertex_shader.glsl", "fragment_shader.glsl");
		lineShader = compileShader("line_vertex_shader.glsl", "line_fragment_shader.glsl");

		genVboVao(shader, {{"pos", 3}, {"col", 4}}, vbo, vao);
		genVboVao(shader, {{"pos", 2}, {"col", 4}}, lineVbo, lineVao);

		//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glEnable(GL_BLEND); // transparency
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	public: ~Renderer() {
		glDeleteVertexArrays(1, &vao);
		glDeleteVertexArrays(1, &lineVao);
		glDeleteBuffers(1, &vbo);
		glDeleteBuffers(1, &lineVbo);
		glfwTerminate();
	}


	public: void setCameraParams(float cameraInclination, float fovy) {
		float screenRatio = (float)width / (float)height;
		int viewUniformLocation = glGetUniformLocation(shader, "view");
		int projectionUniformLocation = glGetUniformLocation(shader, "projection");

		glUseProgram(shader);
		glBindVertexArray(vao);

		// make sure to initialize matrix to identity matrix first
		glm::mat4 view{1.0f};
		//view = glm::translate(view, glm::vec3{0,0,0});
		//view = glm::rotate(view, glm::radians(0.0f), glm::vec3{0,    1.0f, 0}); // yaw
		view = glm::rotate(view, cameraInclination, glm::vec3{1.0f, 0,    0}); // pitch
		glUniformMatrix4fv(viewUniformLocation, 1, GL_FALSE, &view[0][0]);

		glm::mat4 projection = glm::mat4(1.0f);
		projection = glm::perspective(fovy, screenRatio, 0.01f, 100.0f);
		glUniformMatrix4fv(projectionUniformLocation, 1, GL_FALSE, &projection[0][0]);
	}

	public: void loadVertices(const std::vector<float>& vertices) {
		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STREAM_DRAW);
		nrVertices = vertices.size();
	}

	public: void loadLineVertices(const std::vector<float>& vertices) {
		glBindVertexArray(lineVao);
		glBindBuffer(GL_ARRAY_BUFFER, lineVbo);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STREAM_DRAW);
		nrLineVertices = vertices.size();
	}

	public: void setBackgroundColor(const Color& color) {
		backgroundColor = color;
	}


	public: void draw() {
		drawVertices();
		drawLineVertices();
		swapBuffers();
	}

	public: bool shouldClose() {
		return glfwWindowShouldClose(window);
	}

	public: void screenshot(const std::string& filename) {
		drawVertices();
		swapBuffers();
		drawVertices();
		saveScreenshot(0, 0, width, height, filename);
	}
};


std::vector<float> getAnnulus(float x0, float y0, float z0, float internalRadius, float externalRadius, int resolution,
		const std::function<Color()>& colorGenerator) {
	std::vector<float> triangles;
	triangles.reserve(6*resolution);

	auto angle = [&resolution](int v) {
		return 2 * M_PI * v / resolution;
	};
	auto addPoint = [&triangles, &x0, &y0, &z0, &colorGenerator](float radius, float angle) {
		triangles.push_back(x0 + radius*cos(angle));
		triangles.push_back(y0);
		triangles.push_back(z0 + radius*sin(angle));

		auto [r,g,b,a] = colorGenerator();
		triangles.push_back(r);
		triangles.push_back(g);
		triangles.push_back(b);
		triangles.push_back(a);
	};

	for(int v = 0; v != resolution; ++v) {
		float a1 = angle(v);
		float a2 = angle(v+1);

		// first triangle
		addPoint(internalRadius, a1);
		addPoint(internalRadius, a2);
		addPoint(externalRadius, a1);

		// second triangle
		addPoint(internalRadius, a2);
		addPoint(externalRadius, a1);
		addPoint(externalRadius, a2);
	}

	return triangles;
}

std::vector<float> getProjLines(float screenRatio, float cameraInclination, float fovy, const Color& color) {
	auto [r,g,b,a] = color;
	float tanLineAngle = (tan(cameraInclination) / tan(fovy/2) + 1) / screenRatio;
	std::cout<<"Tan line angle = "<<tanLineAngle<<"  -->  line angle = "<<atan(tanLineAngle)<<" rad = "<<glm::degrees(atan(tanLineAngle))<<" degrees\n";

	std::vector<float> result{
		-1,                                -1, r,g,b,a,
		2 / tanLineAngle / screenRatio - 1, 1, r,g,b,a,
		1,                                 -1, r,g,b,a,
		1 - 2 / tanLineAngle / screenRatio, 1, r,g,b,a,
	};

	return result;
}


std::vector<float> merge(const std::initializer_list<std::vector<float>>& vectors) {
	std::vector<float> result;

	for (auto&& v : vectors) {
		result.insert(result.end(), v.begin(), v.end());
	}

	return result;
}

float randomReal() {
	static std::random_device rd;
	static std::mt19937 engine(rd());
	static std::uniform_real_distribution<> dist(0, 1);
	return dist(engine);
}

constexpr Color white() { return {1.0f,1.0f,1.0f}; }
constexpr Color grey() { return {0.05f,0.05f,0.05f}; }
constexpr Color invisible() { return {0.0f,0.0f,0.0f,0.0f}; }

Color alternatingWhite() {
	static int counter = 0;
	++counter;

	if ((counter/15)%7 < 4) {
		return white();
	} else {
		return invisible();
	}
}

Color randomGrey() {
	float grey = randomReal()/10;
	return Color{grey,grey,grey};
}


std::vector<float> getForwardStreetToInfinity(float cameraInclination, float fovy, int width, int height) {
	// To draw triangle representing street to infinity
	auto [re,g,bl,a] = std::tuple{1.0f, 0.0f, 0.0f, 0.15f};
	float r = 0.1f;
	float h = r * (sin(cameraInclination) + cos(cameraInclination) * tan(fovy/2));
	float b = r * (float)width/height * tan(fovy/2);

	return {
		b, -h, 0.0f, re,g,bl,a,
		0, -h, 0.0f, re,g,bl,a,
		b, -h, -10000.0f, re,g,bl,a,
		-b, -h, 0.0f, re,g,bl,a,
		0, -h, 0.0f, re,g,bl,a,
		-b, -h, -10000.0f, re,g,bl,a,
	};
}

std::vector<float> getStreet(double param, const std::function<Color()>& streetColor, const std::function<Color()>& lineColor) {
	int paramSign = (param < 0 ? -1 : 1);
	param = std::pow(std::min(std::max(std::abs(param), 0.01), 1.0), 2);
	double d = 10 * tan(M_PI_2 - param * M_PI_2);

	//std::cout<<"param="<<param<<" sign="<<paramSign<<" d="<<d<<"\n";

	std::vector<float> streets, v0, v1, v2;
	if (paramSign == -1) {
		streets = getAnnulus(-d, -1.501, 0, d-5, d+2, 10000, streetColor);
		v0 =      getAnnulus(-d, -1.5,   0, d-4.6, d-4.4, 10000, lineColor);
		v1 =      getAnnulus(-d, -1.5,   0, d-1.6, d-1.4, 10000, lineColor);
		v2 =      getAnnulus(-d, -1.5,   0, d+1.4, d+1.6, 10000, lineColor);

	} else {
		streets = getAnnulus(d, -1.501, 0, d+5.0, d-2.0, 10000, streetColor);
		v0 =      getAnnulus(d, -1.5,   0, d+4.6, d+4.4, 10000, lineColor);
		v1 =      getAnnulus(d, -1.5,   0, d+1.6, d+1.4, 10000, lineColor);
		v2 =      getAnnulus(d, -1.5,   0, d-1.4, d-1.6, 10000, lineColor);
	}

	return merge({streets,v0,v1,v2});
}


int main() {
	constexpr int width = 1600;
	constexpr int height = 900;
	constexpr float cameraInclination = glm::radians(5.0f);
	constexpr float fovy = glm::radians(41.41f); // pi camera v1
	//constexpr float fovy = glm::radians(48.8f); // pi camera v2
	constexpr Color backgroundColor{0.2f, 0.3f, 0.3f};


	std::vector<float> lineVertices = getProjLines((float)width/height, cameraInclination, fovy, {1.0, 0.0, 0.0});

	Renderer renderer{width, height};
	renderer.setCameraParams(cameraInclination, fovy);
	renderer.setBackgroundColor(backgroundColor);
	renderer.loadLineVertices(lineVertices);

	std::vector<float> street = getStreet(0.3, grey, white);
	renderer.loadVertices(street);
	renderer.screenshot("../screenshot.png");

	while(!renderer.shouldClose()) {
		std::vector<float> street = getStreet(sin(glfwGetTime()/2)/2, grey, white);
		renderer.loadVertices(street);
		renderer.screenshot("../screenshot.png");
		renderer.draw();
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}
}
