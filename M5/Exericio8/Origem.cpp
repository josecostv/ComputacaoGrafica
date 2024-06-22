
/* Hello Triangle - código adaptado de https://learnopengl.com/#!Getting-started/Hello-Triangle
 *
 * Adaptado por Rossana Baptista Queiroz
 * para a disciplina de Processamento Gráfico - Jogos Digitais - Unisinos
 * Versão inicial: 7/4/2017
 * Última atualização em 12/05/2023
 *
 */

#include <iostream>
#include <string>
#include <assert.h>
#include <fstream>
#include <sstream>

using namespace std;

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>


//GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#define STB_IMAGE_IMPLEMENTATION  
#include "../../M3/stb_image.h"

// Protótipo da função de callback do mouse
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

// Protótipo da função de callback de teclado
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);

// Protótipos das funções
int setupShader();
int loadSimpleOBJ(string filepath, int& nVerts, glm::vec3 color);
int loadTexture(string path);
string getTextureName(string filepath, GLuint shaderID);

// Dimensões da janela (pode ser alterado em tempo de execução)
const GLuint WIDTH = 1000, HEIGHT = 1000;

// Código fonte do Vertex Shader (em GLSL): ainda hardcoded
const GLchar* vertexShaderSource = "#version 450\n"
"layout (location = 0) in vec3 position;\n"
"layout (location = 1) in vec3 color;\n"
"layout(location = 2) in vec2 tex_coord;\n"
"layout(location = 3) in vec3 normal;\n"
"uniform mat4 model;\n"
"uniform mat4 view;\n"
"uniform mat4 projection;\n"
"out vec3 finalColor;\n"
"out vec2 texCoord;\n"
"out vec3 fragPos;\n"
"out vec3 scaledNormal;\n"
"void main()\n"
"{\n"
//adicionado ao shader o posicionamento de um segundo cubo por meio do "gl_InstanceID, caso necessário"
"vec3 pos = position;\n"
// "pos.y -= float(gl_InstanceID) * 1.2;\n"	
"gl_Position = projection * view * model * vec4(pos, 1.0);\n"
"finalColor = color;\n"
"scaledNormal = vec3(model * vec4(normal, 1.0));;\n"
"texCoord = vec2(tex_coord.x, 1 - tex_coord.y);\n"
"fragPos = vec3(model * vec4(pos, 1.0));\n"
"}\0";

//Códifo fonte do Fragment Shader (em GLSL): ainda hardcoded
const GLchar* fragmentShaderSource = "#version 450\n"
"in vec3 finalColor;\n"
"in vec2 texCoord;\n"
"in vec3 fragPos;\n"
"in vec3 scaledNormal;\n"
"out vec4 color;\n"
"uniform sampler2D tex_buffer;\n"
"uniform float ka;\n"
"uniform float kd;\n"
"uniform float ks;\n"
"uniform float q;\n"

//Propriedades da fonte de luz
"uniform vec3 lightPos;\n"
"uniform vec3 lightColor;\n"

//Posição da Camera
"uniform vec3 cameraPos;\n"

"void main()\n"
"{\n"
//Cálculo da parcela de iluminação ambiente
"vec3 ambient = ka * lightColor;\n"

//Cálculo da parcela de iluminação difusa
"vec3 N = normalize(scaledNormal);\n"
"vec3 L = normalize(lightPos - fragPos);\n"
"float diff = max(dot(N, L), 0.0);\n"
"vec3 diffuse = kd * diff * lightColor;\n"

//Cálculo da parcela de iluminação especular
"vec3 V = normalize(cameraPos - fragPos);\n"
"vec3 R = normalize(reflect(-L, N));\n"
"float spec = max(dot(R, V), 0.0);\n"
"spec = pow(spec, q);\n"
"vec3 specular = ks * spec * lightColor;\n"

"vec3 result = (ambient + diffuse) * vec3(texture(tex_buffer, texCoord)) + specular;\n"
"color = vec4(result,1.0);\n"
"}\n\0";

bool rotateX = false, rotateY = false, rotateZ = false;

// Adicionadas as variáveis para possibilitar o deslocamento nos três eixos, bem como a alteração da escala

float posX = 0.0f, posY = 0.0f, posZ = 0.0f, scale = 1.0f;
float q, ka, ks, kd;
float fov = 45.0f;

glm::vec3 cameraPos = glm::vec3(0.0, 0.0, 3.0);
glm::vec3 cameraFront = glm::vec3(0.0, 0.0, -1.0);
glm::vec3 cameraUp = glm::vec3(0.0, 1.0, 0.0);

bool firstMouse = true;
float lastX, lastY;
float sensitivity = 0.05;
float pitch = 0.0, yaw = -90.0;

// Função MAIN
int main()
{
	// Inicialização da GLFW
	glfwInit();

	//Muita atenção aqui: alguns ambientes não aceitam essas configurações
	//Você deve adaptar para a versão do OpenGL suportada por sua placa
	//Sugestão: comente essas linhas de código para desobrir a versão e
	//depois atualize (por exemplo: 4.5 com 4 e 5)
	//glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	//glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	//Essencial para computadores da Apple
	//#ifdef __APPLE__
//	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	//#endif

	// Criação da janela GLFW
	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Ola 3D -- José Costa", nullptr, nullptr);
	glfwMakeContextCurrent(window);

	// Fazendo o registro da função de callback para a janela GLFW
	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	// GLAD: carrega todos os ponteiros d funções da OpenGL
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;

	}

	// Obtendo as informações de versão
	const GLubyte* renderer = glGetString(GL_RENDERER); /* get renderer string */
	const GLubyte* version = glGetString(GL_VERSION); /* version as a string */
	cout << "Renderer: " << renderer << endl;
	cout << "OpenGL version supported " << version << endl;

	// Definindo as dimensões da viewport com as mesmas dimensões da janela da aplicação
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);


	// Compilando e buildando o programa de shader
	GLuint shaderID = setupShader();

	// Leitura do arquivo "cube.obj" para obter a textura
	string textureName = getTextureName("cube.mtl", shaderID);

	GLuint texID = loadTexture("cube.png");

	int nVerts = 36;

	// Leitura do arquivo "cube.obj" para obter a geometria
	GLuint VAO = loadSimpleOBJ("cube.obj", nVerts, glm::vec3(0, 1, 0));


	glUseProgram(shaderID);

	//Matriz de view -- posição e orientação da câmera
	glm::mat4 view = glm::lookAt(glm::vec3(0.0, 0.0, 3.0), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
	GLint viewLoc = glGetUniformLocation(shaderID, "view");
	glUniformMatrix4fv(viewLoc, 1, FALSE, value_ptr(view));

	glm::mat4 model = glm::mat4(1); //matriz identidade;
	GLint modelLoc = glGetUniformLocation(shaderID, "model");
	//
	model = glm::rotate(model, /*(GLfloat)glfwGetTime()*/glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	glUniformMatrix4fv(modelLoc, 1, FALSE, glm::value_ptr(model));

	glEnable(GL_DEPTH_TEST);

	//Definindo as propriedades do material da superficie
	glUniform1f(glGetUniformLocation(shaderID, "ka"), ka);
	glUniform1f(glGetUniformLocation(shaderID, "kd"), kd);
	glUniform1f(glGetUniformLocation(shaderID, "ks"), ks);
	glUniform1f(glGetUniformLocation(shaderID, "q"), q);

	//Definindo a fonte de luz pontual
	glUniform3f(glGetUniformLocation(shaderID, "lightPos"), -2.0, 5.0, 10.0);
	glUniform3f(glGetUniformLocation(shaderID, "lightColor"), 1.0, 1.0, 0.0);

	// Loop da aplicação - "game loop"
	while (!glfwWindowShouldClose(window))
	{
		// Checa se houveram eventos de input (key pressed, mouse moved etc.) e chama as funções de callback correspondentes
		glfwPollEvents();

		// Limpa o buffer de cor
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f); //cor de fundo
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glLineWidth(10);
		glPointSize(20);

		glm::mat4 projection = glm::perspective(glm::radians(fov), (float)width / (float)height, 0.1f, 100.0f);
		GLint projectionLoc = glGetUniformLocation(shaderID, "projection");
		glUniformMatrix4fv(projectionLoc, 1, FALSE, value_ptr(projection));

		//Atualizando a posição e orientação da câmera
		glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
		glUniformMatrix4fv(glGetUniformLocation(shaderID, "view"), 1, FALSE, value_ptr(view));

		//Atualizando o shader com a posição da câmera
		glUniformMatrix3fv(glGetUniformLocation(shaderID, "cameraPos"), 1, FALSE, value_ptr(glm::vec3(cameraPos.x, cameraPos.y, cameraPos.z)));

		
		float angle = (GLfloat)glfwGetTime();

		// Adicionadas matrizes de translação e escala
		model = glm::mat4(1);
		model = glm::translate(model, glm::vec3(posX, posY, posZ));
		model = glm::scale(model, glm::vec3(scale));
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


		glUniformMatrix4fv(modelLoc, 1, FALSE, glm::value_ptr(model));
		// Chamada de desenho - drawcall
		// Poligono Preenchido - GL_TRIANGLES

		// Ativação do buffer de textura e realização do bind com o buffer
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texID);

		glBindVertexArray(VAO);
		glDrawArraysInstanced(GL_TRIANGLES, 0, 36, 1);

		// Chamada de desenho - drawcall
		// CONTORNO - GL_LINE_LOOP

		// glDrawArrays(GL_POINTS, 0, 36);
		glBindVertexArray(0);

		// Troca os buffers da tela
		glfwSwapBuffers(window);
	}
	// Pede pra OpenGL desalocar os buffers
	glDeleteVertexArrays(1, &VAO);
	// Finaliza a execução da GLFW, limpando os recursos alocados por ela
	glfwTerminate();
	return 0;
}

// Função de callback de teclado - só pode ter uma instância (deve ser estática se
// estiver dentro de uma classe) - É chamada sempre que uma tecla for pressionada
// ou solta via GLFW
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

	//Adicionados os comandos para a movimentação nos três eixos, além da alteração da escala por meio das teclas de "-" e "+"
	if (key == GLFW_KEY_W && action == GLFW_PRESS) {
		posY += 0.1f;
	}
	if (key == GLFW_KEY_S && action == GLFW_PRESS) {
		posY -= 0.1f;
	}
	if (key == GLFW_KEY_A && action == GLFW_PRESS) {
		posX -= 0.1f;
	}
	if (key == GLFW_KEY_D && action == GLFW_PRESS) {
		posX += 0.1f;
	}
	if (key == GLFW_KEY_J && action == GLFW_PRESS) {
		posZ += 0.1f;
	}
	if (key == GLFW_KEY_I && action == GLFW_PRESS) {
		posZ -= 0.1f;
	}
	if (key == GLFW_KEY_MINUS && action == GLFW_PRESS) {
		scale -= 0.2;
	}
	if (key == GLFW_KEY_EQUAL && action == GLFW_PRESS) {
		scale += 0.2;
	}

	float cameraSpeed = 0.05;

	if (key == GLFW_KEY_O)
	{
		cameraPos += cameraFront * cameraSpeed;
	}
	if (key == GLFW_KEY_J)
	{
		cameraPos -= cameraFront * cameraSpeed;
	}
	if (key == GLFW_KEY_K)
	{
		cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
	}
	if (key == GLFW_KEY_L)
	{
		cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
	}

}

//Esta função está basntante hardcoded - objetivo é compilar e "buildar" um programa de
// shader simples e único neste exemplo de código
// O código fonte do vertex e fragment shader está nos arrays vertexShaderSource e
// fragmentShader source no iniçio deste arquivo
// A função retorna o identificador do programa de shader
int setupShader()
{
	// Vertex shader
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);
	// Checando erros de compilação (exibição via log no terminal)
	GLint success;
	GLchar infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
	}
	// Fragment shader
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);
	// Checando erros de compilação (exibição via log no terminal)
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
	}
	// Linkando os shaders e criando o identificador do programa de shader
	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	// Checando por erros de linkagem
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
	}
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return shaderProgram;
}

// Adição da função "loadSimpleOBJ"
int loadSimpleOBJ(string filepath, int& nVerts, glm::vec3 color)
{
	vector <glm::vec3> vertices;
	vector <GLuint> indices;
	vector <glm::vec2> texCoords;
	vector <glm::vec3> normals;
	vector <GLfloat> vbuffer;

	// Leitura do arquivo

	ifstream inputFile;
	inputFile.open(filepath.c_str());
	if (inputFile.is_open())
	{
		char line[100];
		string sline;



		while (!inputFile.eof())
		{
			inputFile.getline(line, 100);
			sline = line;

			string word;

			istringstream ssline(line);
			ssline >> word;
			//cout << word << " ";
			//Mapeando as posições
			if (word == "v")
			{
				glm::vec3 v;
				ssline >> v.x >> v.y >> v.z;


				vertices.push_back(v);
			}
			//Mapeando as coordenadas da textura
			if (word == "vt")
			{
				glm::vec2 vt;
				ssline >> vt.s >> vt.t;

				texCoords.push_back(vt);
			}
			//Mapeando as coordenadas dos vetores normais
			if (word == "vn")
			{
				glm::vec3 vn;
				ssline >> vn.x >> vn.y >> vn.z;
				normals.push_back(vn);
			}
			if (word == "f")
			{
				string tokens[3];

				ssline >> tokens[0] >> tokens[1] >> tokens[2];

				for (int i = 0; i < 3; i++)
				{
					//Recuperando os indices de v
					int pos = tokens[i].find("/");
					string token = tokens[i].substr(0, pos);
					int index = atoi(token.c_str()) - 1;
					indices.push_back(index);

					vbuffer.push_back(vertices[index].x);
					vbuffer.push_back(vertices[index].y);
					vbuffer.push_back(vertices[index].z);

					vbuffer.push_back(color.r);
					vbuffer.push_back(color.g);
					vbuffer.push_back(color.b);

					//Recuperando os indices de vts
					tokens[i] = tokens[i].substr(pos + 1);
					pos = tokens[i].find("/");
					token = tokens[i].substr(0, pos);
					index = atoi(token.c_str()) - 1;
					vbuffer.push_back(texCoords[index].s);
					vbuffer.push_back(texCoords[index].t);

					//Recuperando os indices de vns
					tokens[i] = tokens[i].substr(pos + 1);
					index = atoi(tokens[i].c_str()) - 1;
					vbuffer.push_back(normals[index].x);
					vbuffer.push_back(normals[index].y);
					vbuffer.push_back(normals[index].z);
				}
			}
		}
	}
	else
	{
		cout << "Problema ao encontrar o arquivo " << filepath << endl;
	}
	inputFile.close();
	GLuint VBO, VAO;
	nVerts = vbuffer.size() / 11; // 3 pos + 3 cor + 3 normal + 2 texcoord
	//Geração do identificador do VBO
	glGenBuffers(1, &VBO);
	//Faz a conexão (vincula) do buffer como um buffer de array
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	//Envia os dados do array de floats para o buffer da OpenGl
	glBufferData(GL_ARRAY_BUFFER, vbuffer.size() * sizeof(GLfloat), vbuffer.data(), GL_STATIC_DRAW);
	//Geração do identificador do VAO (Vertex Array Object)
	glGenVertexArrays(1, &VAO);
	// Vincula (bind) o VAO primeiro, e em seguida  conecta e seta o(s) buffer(s) de vértices
	// e os ponteiros para os atributos 
	glBindVertexArray(VAO);
	//Para cada atributo do vertice, criamos um "AttribPointer" (ponteiro para o atributo), indicando: 
	// Localização no shader * (a localização dos atributos devem ser correspondentes no layout especificado no vertex shader)
	// Numero de valores que o atributo tem (por ex, 3 coordenadas xyz) 
	// Tipo do dado
	// Se está normalizado (entre zero e um)
	// Tamanho em bytes 
	// Deslocamento a partir do byte zero 
	//Atributo posição (x, y, z)
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);
	//Atributo cor (r, g, b)
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);
	//Atributo coordenada de textura (s, t)
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
	glEnableVertexAttribArray(2);
	//Atributo normal do vértice (x, y, z)
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(8 * sizeof(GLfloat)));
	glEnableVertexAttribArray(3);

	// Observe que isso é permitido, a chamada para glVertexAttribPointer registrou o VBO como o objeto de buffer de vértice 
	// atualmente vinculado - para que depois possamos desvincular com segurança
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	// Desvincula o VAO (é uma boa prática desvincular qualquer buffer ou array para evitar bugs medonhos)
	glBindVertexArray(0);
	return VAO;
}

// Adição da função "loadTexture"
int loadTexture(string path)
{
	GLuint texID;

	// Gera o identificador da textura na memória 
	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);

	//Ajusta os parâmetros de wrapping e filtering
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	//Carregamento da imagem
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

	return texID;
}

// Função que lê o arquivo passado e retorna o nome da textura
string getTextureName(string filepath, GLuint shaderID)
{

	ifstream inputFile;

	inputFile.open(filepath.c_str());

	string textureName;

	if (inputFile.is_open())
	{
		char line[100];
		string sline;


		while (!inputFile.eof())
		{
			inputFile.getline(line, 100);
			sline = line;

			string word;

			istringstream ssline(line);
			ssline >> word;

			//cout << word << " ";
			if (word == "map_Kd")
			{
				string tokens[1];
				ssline >> tokens[0];

				glm::vec3 v;
				ssline >> v.x >> v.y >> v.z;
				textureName = tokens[0];
			}
			if (word == "Ns")
			{
				string tokens[1];
				ssline >> tokens[0];
				string value = tokens[0];

				float parseFloat = std::stof(value);

				q = parseFloat;
			}
			if (word == "Ka")
			{
				string tokens[1];
				ssline >> tokens[0];
				string value = tokens[0];

				float parseFloat = std::stof(value);

				ka = parseFloat;
			}
			if (word == "Ks")
			{
				string tokens[1];
				ssline >> tokens[0];
				string value = tokens[0];

				float parseFloat = std::stof(value);

				ks = parseFloat;
			}
			if (word == "Kd")
			{
				string tokens[1];
				ssline >> tokens[0];
				string value = tokens[0];

				float parseFloat = std::stof(value);

				kd = parseFloat;
			}

		}
	}
	else
	{
		cout << "Problema ao encontrar o arquivo " << filepath << endl;
	}
	inputFile.close();

	return textureName;
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos;
	lastX = xpos;
	lastY = ypos;

	float sensitivity = 0.1f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	yaw += xoffset;
	pitch += yoffset;

	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;

	glm::vec3 direction;
	direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	direction.y = sin(glm::radians(pitch));
	direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	cameraFront = glm::normalize(direction);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	cout << xoffset;
	fov -= (float)yoffset;
	if (fov < 1.0f)
		fov = 1.0f;
	if (fov > 45.0f)
		fov = 45.0f;
}