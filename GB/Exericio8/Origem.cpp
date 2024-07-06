#include <iostream>
#include <string>
#include <assert.h>
#include <fstream>
#include <vector>
#include <sstream>
#include <map>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "stb_image.h"
#include "Shader.h"
#include "Bezier.h"

using namespace std;

const GLuint WIDTH = 1000, HEIGHT = 1000;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void moveObject(glm::mat4& model, glm::vec3 coord);
void loadMtl(string filename, map<string, string>& properties);
int loadObj(string objFile, int& verticesSize);
int loadTexture(string path);
float stringToFloat(string value, float def);
vector<glm::vec3> createControlPoints(string filename);
vector<string> split(const string& input, char delimiter);

bool rotateX = false, rotateY = false, rotateZ = false, firstMouse = true;
float lastX, lastY, sensitivity = 0.05, pitch = 0.0, yaw = -90.0;
vector<int> objectsMovementControl;

int shieldVertSize = 0, SHIELD_MOVE_KEY = GLFW_KEY_1;
int ballVerticesSize = 0, MEMORY_CARD_MOVE_KEY = GLFW_KEY_2;

glm::vec3 cameraPos = glm::vec3(0.0, 0.0, 10.0);
glm::vec3 cameraFront = glm::vec3(0.0, 0.0, -1.0);
glm::vec3 cameraUp = glm::vec3(0.0, 1.0, 0.0);

struct Vertex {
	float x, y, z, r = 0.1f, g = 0.1f, b = 0.1f;
};

struct Texture {
	float s, t;
};

struct Normal {
	float x, y, z;
};

struct Face {
	Vertex vertices[3];
	Texture textures[3];
	Normal normals[3];
};

int main()
{
	glfwInit();
	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "GB - Jose Costa", nullptr, nullptr);
	glfwMakeContextCurrent(window);
	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetCursorPos(window, WIDTH / 2, HEIGHT / 2);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		cout << "Failed to initialize GLAD" << endl;
	}

	const GLubyte* renderer = glGetString(GL_RENDERER);
	const GLubyte* version = glGetString(GL_VERSION);
	cout << "Renderer: " << renderer << endl;
	cout << "OpenGL version supported " << version << endl;

	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);

	Shader shader("../shaders/shaders.vs", "../shaders/shaders.fs");

	glUseProgram(shader.ID);

	glm::mat4 view = glm::lookAt(glm::vec3(0.0, 0.0, 3.0), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
	shader.setMat4("view", value_ptr(view));
	glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 100.0f);
	shader.setMat4("projection", glm::value_ptr(projection));
	glm::mat4 model = glm::mat4(1);
	model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	shader.setMat4("model", glm::value_ptr(model));
	glUniform1i(glGetUniformLocation(shader.ID, "tex_buffer"), 0);

	shader.setVec3("lightPosition", 15.0f, 15.0f, 2.0f);
	shader.setVec3("lightColor", 1.0f, 1.0f, 1.0f);

	// Loading the 3D Models
	GLuint shieldVAO = loadObj("../3d-models/shield/Shield.obj", shieldVertSize);
	map<string, string> shieldProps;
	loadMtl("../3d-models/shield/Shield.mtl", shieldProps);
	GLuint textShield = loadTexture(shieldProps["map_Kd"]);

	GLuint memoryCardVAO = loadObj("../3d-models/memory-card/MemoryCard.obj", ballVerticesSize);
	map<string, string> memoryCardProps;
	loadMtl("../3d-models/memory-card/MemoryCard.mtl", memoryCardProps);
	GLuint texMemoryCard = loadTexture(memoryCardProps["map_Kd"]);

	glEnable(GL_DEPTH_TEST);

	vector<glm::vec3> controlPoints = createControlPoints("../curves.txt");

	Bezier bezier;
	bezier.setControlPoints(controlPoints);
	bezier.setShader(&shader);
	bezier.generateCurve(1200);

	int nbCurvePoints = bezier.getNbCurvePoints();
	int i = 0;

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glLineWidth(10);
		glPointSize(20);

		model = glm::mat4(1);

		glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
		shader.setMat4("view", glm::value_ptr(view));
		shader.setVec3("cameraPos", cameraPos.x, cameraPos.y, cameraPos.z);
		shader.setMat4("model", glm::value_ptr(model));

		// ##############
		// SHIELD SECTION
		// ##############
		glBindVertexArray(shieldVAO);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textShield);

		model = glm::mat4(1);
		model = glm::scale(model, glm::vec3(2.0, 2.0, 2.0));
		model = glm::rotate(model, glm::radians(45.0f), glm::vec3(1.0f, 1.0f, 1.0f));

		if (find(objectsMovementControl.begin(), objectsMovementControl.end(), SHIELD_MOVE_KEY) != objectsMovementControl.end())
		{
			moveObject(model, bezier.getPointOnCurve(i));
		}

		shader.setMat4("model", glm::value_ptr(model));

		shader.setFloat("ka", stringToFloat(shieldProps["Ka"], 0));
		shader.setFloat("kd", stringToFloat(shieldProps["Kd"], 1.5));
		shader.setFloat("ks", stringToFloat(shieldProps["Ks"], 0));
		shader.setFloat("q", stringToFloat(shieldProps["Ns"], 0));

		glDrawArrays(GL_TRIANGLES, 0, shieldVertSize);

		// ###################
		// MEMORY CARD SECTION
		// ###################
		glBindVertexArray(memoryCardVAO);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texMemoryCard);

		model = glm::mat4(1);
		model = glm::scale(model, glm::vec3(1.0, 1.0, 1.0));
		model = glm::translate(model, glm::vec3(4.5, -1.0, 0.0));
		model = glm::rotate(model, glm::radians(45.0f), glm::vec3(1.0f, 1.0f, 1.0f));

		if (find(objectsMovementControl.begin(), objectsMovementControl.end(), MEMORY_CARD_MOVE_KEY) != objectsMovementControl.end())
		{
			moveObject(model, bezier.getPointOnCurve(i));
		}

		shader.setMat4("model", glm::value_ptr(model));
		shader.setFloat("ka", stringToFloat(memoryCardProps["Ka"], 0));
		shader.setFloat("kd", stringToFloat(memoryCardProps["Kd"], 1.5));
		shader.setFloat("ks", stringToFloat(memoryCardProps["Ks"], 0));
		shader.setFloat("q", stringToFloat(memoryCardProps["Ns"], 0));

		glDrawArrays(GL_TRIANGLES, 0, ballVerticesSize);

		i = (i + 1) % nbCurvePoints;

		glBindVertexArray(0);
		glBindTexture(GL_TEXTURE_2D, 0);

		glfwSwapBuffers(window);
	}

	glDeleteVertexArrays(1, &shieldVAO);
	glDeleteVertexArrays(1, &memoryCardVAO);

	glfwTerminate();

	return 0;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	if (key == GLFW_KEY_X && action == GLFW_PRESS)
	{
		rotateX = true;
		rotateY = false;
		rotateZ = false;
	}

	if (key == GLFW_KEY_Y && action == GLFW_PRESS)
	{
		rotateX = false;
		rotateY = true;
		rotateZ = false;
	}

	if (key == GLFW_KEY_Z && action == GLFW_PRESS)
	{
		rotateX = false;
		rotateY = false;
		rotateZ = true;
	}

	float cameraSpeed = 0.05f;

	if (key == GLFW_KEY_W && action == GLFW_REPEAT)
	{
		cameraPos += cameraFront * cameraSpeed;
	}

	if (key == GLFW_KEY_A && action == GLFW_REPEAT)
	{
		cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
	}

	if (key == GLFW_KEY_S && action == GLFW_REPEAT)
	{
		cameraPos -= cameraFront * cameraSpeed;
	}

	if (key == GLFW_KEY_D && action == GLFW_REPEAT)
	{
		cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
	}

	if ((key >= GLFW_KEY_0 && key <= GLFW_KEY_9) && action == GLFW_PRESS) {
		if (find(objectsMovementControl.begin(), objectsMovementControl.end(), key) != objectsMovementControl.end()) {
			objectsMovementControl.erase(remove(objectsMovementControl.begin(), objectsMovementControl.end(), key), objectsMovementControl.end());
		} else {
			objectsMovementControl.push_back(key);
		}
	}
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float offsetx = xpos - lastX;
	float offsety = lastY - ypos;

	lastX = xpos;
	lastY = ypos;

	offsetx *= sensitivity;
	offsety *= sensitivity;

	pitch += offsety;
	yaw += offsetx;

	glm::vec3 front;

	front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	front.y = sin(glm::radians(pitch));
	front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

	cameraFront = glm::normalize(front);
}

int loadObj(string filename, int& verticesSize)
{
	vector<float> vertices;

	ifstream file(filename);

	vector<Vertex> uniqueVertices;
	vector<Texture> uniqueTextures;
	vector<Normal> uniqueNormals;
	vector<Face> faces;

	vector<float> buffer;

	if (file.is_open()) {
		string line;

		while (getline(file, line)) {
			vector<string> row = split(line, ' ');

			bool isVertice = row[0] == "v";
			bool isFace = row[0] == "f";
			bool isTexture = row[0] == "vt";
			bool isNormal = row[0] == "vn";

			if (isVertice) {
				float x = stof(row[1]), y = stof(row[2]), z = stof(row[3]);

				Vertex vertex;

				vertex.x = x;
				vertex.y = y;
				vertex.z = z;

				uniqueVertices.push_back(vertex);
			}

			if (isTexture) {
				float x = stof(row[1]), y = stof(row[2]);

				Texture texture;

				texture.s = x;
				texture.t = y;

				uniqueTextures.push_back(texture);
			}

			if (isNormal) {
				float x = stof(row[1]), y = stof(row[2]), z = stof(row[3]);

				Normal normal;

				normal.x = x;
				normal.y = y;
				normal.z = z;

				uniqueNormals.push_back(normal);
			}

			if (isFace) {
				vector<string> x = split(row[1], '/'), y = split(row[2], '/'), z = split(row[3], '/');

				int v1 = stoi(x[0]) - 1, v2 = stoi(y[0]) - 1, v3 = stoi(z[0]) - 1;
				int t1 = stoi(x[1]) - 1, t2 = stoi(y[1]) - 1, t3 = stoi(z[1]) - 1;
				int n1 = stoi(x[2]) - 1, n2 = stoi(y[2]) - 1, n3 = stoi(z[2]) - 1;

				Face face;

				face.vertices[0] = uniqueVertices[v1];
				face.textures[0] = uniqueTextures[t1];
				face.normals[0] = uniqueNormals[n1];

				face.vertices[1] = uniqueVertices[v2];
				face.textures[1] = uniqueTextures[t2];
				face.normals[1] = uniqueNormals[n2];

				face.vertices[2] = uniqueVertices[v3];
				face.textures[2] = uniqueTextures[t3];
				face.normals[2] = uniqueNormals[n3];

				faces.push_back(face);
			}
		}

		file.close();

		for (Face face : faces)
		{
			for (int i = 0; i < 3; i++)
			{
				buffer.push_back(face.vertices[i].x);
				buffer.push_back(face.vertices[i].y);
				buffer.push_back(face.vertices[i].z);

				buffer.push_back(face.vertices[i].r);
				buffer.push_back(face.vertices[i].g);
				buffer.push_back(face.vertices[i].b);

				buffer.push_back(face.textures[i].s);
				buffer.push_back(face.textures[i].t);

				buffer.push_back(face.normals[i].x);
				buffer.push_back(face.normals[i].y);
				buffer.push_back(face.normals[i].z);
			}
		}

		vertices = buffer;
	}
	else {
		cout << "Unable to open the file: " << filename << endl;
	}
	verticesSize = vertices.size();
	GLuint VBO, VAO;
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(8 * sizeof(GLfloat)));
	glEnableVertexAttribArray(3);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	return VAO;
}

vector<glm::vec3> createControlPoints(string filename)
{
	ifstream file(filename);
	vector <glm::vec3> points;

	if (file.is_open()) {
		string line;

		while (getline(file, line)) {
			vector<string> row = split(line, ',');

			if (row.empty()) {
				continue;
			}

			glm::vec3 point;
			point.x = stof(row[0]);
			point.y = stof(row[1]);
			point.z = stof(row[2]);

			points.push_back(point);
		}

		file.close();
	}
	else {
		cout << "Unable to open the file: " << filename << endl;
	}

	return points;
}

vector<string> split(const string& input, char delimiter) {
	vector<string> tokens;
	istringstream iss(input);
	string token;

	while (getline(iss, token, delimiter)) {
		tokens.push_back(token);
	}

	return tokens;
}

int loadTexture(string path)
{
	GLuint tex;

	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	int width, height, nrChannels;
	unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);

	if (data)
	{
		if (nrChannels == 3) //jpg, bmp
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		}
		else //png
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		}
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}

	stbi_image_free(data);

	glBindTexture(GL_TEXTURE_2D, 0);

	return tex;
}

void loadMtl(string filename, map<string, string>& properties) {
	ifstream file(filename);

	if (file.is_open()) {
		string line;

		while (getline(file, line)) {
			vector<string> row = split(line, ' ');

			if (row.size() == 0) {
				continue;
			}

			properties[row[0]] = row[1];
		}

		file.close();
	}
	else {
		cout << "Unable to open the file: " << filename << endl;
	}
}

// Utilizado para previnir conversão de valor nulo
float stringToFloat(string value, float def) {
	if (value.empty()) {
		return def;
	}
	return stof(value);
}

void moveObject(glm::mat4& model, glm::vec3 coord) {
	float angle = (GLfloat)glfwGetTime();

	model = glm::translate(model, coord);

	if (rotateX)
	{
		model = glm::rotate(model, angle, glm::vec3(1.0f, 0.0f, 0.0f));

	}
	else if (rotateY)
	{
		model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));

	}
	else if (rotateZ)
	{
		model = glm::rotate(model, angle, glm::vec3(0.0f, 0.0f, 1.0f));
	}
}