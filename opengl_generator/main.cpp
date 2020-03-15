/*
g++ -std=c++17 -Iglad/include -ITinyPngOut/include main.cpp glad/src/glad.c TinyPngOut/src/TinyPngOut.cpp -lSOIL -lstdc++fs -lGL -lGLU -lglfw3 -lX11 -lXxf86vm -lXrandr -lpthread -lXi -ldl -lXinerama -lXcursor && ./a.out
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


std::string getFileContent(std::string filename) {
	std::ifstream file(filename);
	return std::string(std::istreambuf_iterator<char>(file),
		std::istreambuf_iterator<char>());
}


GLFWwindow* init(unsigned int w, unsigned int h) {
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // fix compilation on OS X
	#endif

	// glfw window creation
	GLFWwindow* window = glfwCreateWindow(w, h, "LearnOpenGL", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return NULL;
	}
	glfwMakeContextCurrent(window);

	// glad: load all OpenGL function pointers
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return NULL;
	}

	return window;
}


void checkShader(int shader, const std::string& name) {
	int success;
	char infoLog[512];
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(shader, 512, NULL, infoLog);
		std::cout << name << " shader compilation failed:\n" << infoLog << "\n";
	}
}

void checkProgram(int program) {
	int success;
	char infoLog[512];
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(program, 512, NULL, infoLog);
		std::cout << "program linking failed:\n" << infoLog << std::endl;
	}
}

int compileShader(const std::string& vertexFilename, const std::string& fragmentFilename) {
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


auto genVboVao(unsigned int shader, const std::vector<float>& vertices,
		const std::vector<std::pair<std::string, int>>& attribs) {
	unsigned int vbo_id, vao_id;
	glGenVertexArrays(1, &vao_id); // predisponimi un VAO e salva un identificatore in `vao_id`
	glGenBuffers(1, &vbo_id); // predisponimi un VBO e salva un identificatore in `vbo_id`

	glBindVertexArray(vao_id); // voglio usare il VAO all'id `vao_id`
	glBindBuffer(GL_ARRAY_BUFFER, vbo_id); // voglio usare il VBO all'id `vbo_id`
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW); // carica i dati usando il VBO attivo


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


	glBindBuffer(GL_ARRAY_BUFFER, 0); // non uso piu' il VBO
	glBindVertexArray(0); // non uso piu' il VAO
	return std::pair{vbo_id, vao_id};
}


std::vector<float> getAnnulus(float x0, float y0, float z0, float internalRadius, float externalRadius, int resolution,
		const std::function<std::tuple<float,float,float>()>& colorGenerator) {
	std::vector<float> triangles;
	triangles.reserve(6*resolution);

	auto angle = [&resolution](int v) {
		return 2 * M_PI * v / resolution;
	};
	auto addPoint = [&triangles, &x0, &y0, &z0, &colorGenerator](float radius, float angle) {
		triangles.push_back(x0 + radius*cos(angle));
		triangles.push_back(y0);
		triangles.push_back(z0 +radius*sin(angle));

		auto [r,g,b] = colorGenerator();
		triangles.push_back(r);
		triangles.push_back(g);
		triangles.push_back(b);
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


void draw(GLFWwindow* window, float screenRatio, int shader, int vao, int nrVertices, float cameraInclination, float r, float g, float b) {
	int viewUniformLocation = glGetUniformLocation(shader, "view");
	int projectionUniformLocation = glGetUniformLocation(shader, "projection");

	glClearColor(r, g, b, 1.0f); // il colore di background
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // disegnare solo il perimetro dei triangoli
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // also clear the depth buffer now!

	glUseProgram(shader);
	glBindVertexArray(vao);

	// make sure to initialize matrix to identity matrix first
	glm::mat4 view{1.0f};
	//view = glm::translate(view, glm::vec3{0,0,0});
	//view = glm::rotate(view, glm::radians(0.0f), glm::vec3{0,    1.0f, 0}); // yaw
	view = glm::rotate(view, glm::radians(cameraInclination), glm::vec3{1.0f, 0,    0}); // pitch
	glUniformMatrix4fv(viewUniformLocation, 1, GL_FALSE, &view[0][0]);

	glm::mat4 projection = glm::mat4(1.0f);
	projection = glm::perspective(glm::radians(45.0f), screenRatio, 0.01f, 100.0f);
	glUniformMatrix4fv(projectionUniformLocation, 1, GL_FALSE, &projection[0][0]);

	glDrawArrays(GL_TRIANGLES, 0, nrVertices);
	glfwSwapBuffers(window);
}


void screenshot(int x, int y, unsigned int w, unsigned int h) {
	std::vector<uint8_t> pixels(3 * w * h);
	glReadPixels(x, y, w, h, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

	for(int line = 0; line != h/2; ++line) {
		std::swap_ranges(	pixels.begin() + 3 * w * line,
								pixels.begin() + 3 * w * (line+1),
								pixels.begin() + 3 * w * (h-line-1));
	}

	std::ofstream screenshotFile{"screenshot.png", std::ios::binary};
	TinyPngOut{w, h, screenshotFile}.write(pixels.data(), w * h);
}


void show(unsigned int width, unsigned int height, std::vector<float> vertices, float cameraInclination, float r, float g, float b) {
	GLFWwindow* window = init(width, height);
	if(window == NULL) return;

	int shader = compileShader("vertex_shader.glsl", "fragment_shader.glsl");
	auto [vbo, vao] = genVboVao(shader, vertices, {{"pos", 3}, {"col", 3}});

	draw(window, (float)width/height, shader, vao, vertices.size(),
			cameraInclination, r, g, b);

	screenshot(0, 0, width, height);


	while (!glfwWindowShouldClose(window)) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		glfwPollEvents();
	}

	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &vbo);
	glfwTerminate();
}

std::vector<float> merge(const std::initializer_list<std::vector<float>>& vectors) {
	std::vector<float> result;

	for (auto&& v : vectors) {
		result.insert(result.end(), v.begin(), v.end());
	}

	return result;
}


int main() {
	std::vector<float> v1 = getAnnulus(-100, -0.5, 0, 99.9, 100.1, 1000,
			[]() { return std::tuple{0.5f,0.0f,1.0f}; });

	std::vector<float> v2 = getAnnulus(-100, -0.4, 0, 99.8, 100.2, 1000,
			[]() { return std::tuple{1.0f,0.5f,0.0f}; });

	std::vector<float> vertices = merge({v1, v2});

	show(1600, 900, vertices, 5.0f, 0.2f, 0.3f, 0.3f);
}
