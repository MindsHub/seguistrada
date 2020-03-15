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

// settings
constexpr int SCR_WIDTH = 1600;
constexpr int SCR_HEIGHT = 900;


std::string getFileContent(std::string filename) {
	std::ifstream file(filename);
	return std::string(std::istreambuf_iterator<char>(file),
		std::istreambuf_iterator<char>());
}


GLFWwindow* init() {
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // fix compilation on OS X
	#endif

	// glfw window creation
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
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

glm::mat4 getViewMatrix(const glm::vec3& position, float yaw, float pitch) {
	constexpr glm::vec3 worldUp{0.0f, 1.0f, 0.0f};

	glm::vec3 front;
	front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	front.y = sin(glm::radians(pitch));
	front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	front = glm::normalize(front);

	glm::vec3 right = glm::normalize(glm::cross(front, worldUp));
	glm::vec3 up = glm::normalize(glm::cross(right, front));

	return glm::lookAt(position, position + front, up);
}


void screenshot() {
	std::vector<uint8_t> pixels(3 * SCR_WIDTH * SCR_HEIGHT);
	glReadPixels(0, 0, SCR_WIDTH, SCR_HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

	for(int h = 0; h != SCR_HEIGHT/2; ++h) {
		std::swap_ranges(	pixels.begin() + 3 * SCR_WIDTH * h,
								pixels.begin() + 3 * SCR_WIDTH * (h+1),
								pixels.begin() + 3 * SCR_WIDTH * (SCR_HEIGHT-h-1));
	}

	std::ofstream screenshotFile{"screenshot.png", std::ios::binary};
	TinyPngOut{SCR_WIDTH, SCR_HEIGHT, screenshotFile}.write(pixels.data(), SCR_WIDTH * SCR_HEIGHT);
}


int main() {
	GLFWwindow* window = init();
	if(window == NULL) return 1;


	int shader = compileShader("vertex_shader.glsl", "fragment_shader.glsl");

	std::vector<float> vertices = getAnnulus(-100, -0.5, 0, 99.9, 100.1, 1000,
		[]() { return std::tuple{0.5f,0.0f,1.0f}; });

	auto [vbo, vao] = genVboVao(shader, vertices, {{"pos", 3}, {"col", 3}});


	int viewUniformLocation = glGetUniformLocation(shader, "view");
	int projectionUniformLocation = glGetUniformLocation(shader, "projection");


	glClearColor(0.2f, 0.3f, 0.3f, 1.0f); // il colore di background
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // disegnare solo il perimetro dei triangoli
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // also clear the depth buffer now!

	glUseProgram(shader);
	glBindVertexArray(vao);

	// make sure to initialize matrix to identity matrix first
	glm::mat4 view{1.0f};
	//view = glm::translate(view, glm::vec3{0,0,0});
	//view = glm::rotate(view, glm::radians(0.0f), glm::vec3{0,    1.0f, 0}); // yaw
	view = glm::rotate(view, glm::radians(5.0f), glm::vec3{1.0f, 0,    0}); // pitch
	glUniformMatrix4fv(viewUniformLocation, 1, GL_FALSE, &view[0][0]);

	glm::mat4 projection = glm::mat4(1.0f);
	projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.01f, 100.0f);
	glUniformMatrix4fv(projectionUniformLocation, 1, GL_FALSE, &projection[0][0]);

	glDrawArrays(GL_TRIANGLES, 0, vertices.size());
	glfwSwapBuffers(window);


	screenshot();


	while (!glfwWindowShouldClose(window)) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		glfwPollEvents();
	}



	///////////
	// FINE
	///////////

	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &vbo);
	glfwTerminate();
	return 0;
}
