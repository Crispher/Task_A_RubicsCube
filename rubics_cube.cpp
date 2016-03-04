#define GLM_SWIZZLE
#include "stdafx.h"
#define STB_IMAGE_IMPLEMENTATION
#include "util.h"
#include "glm/gtc/matrix_transform.hpp"

namespace rc {
	const int MAX_TILES = 3000;
	int dim = 3;

	glm::vec3 tiles[MAX_TILES];
	int tile_cursor;
	glm::vec2 textures[MAX_TILES];
	int tex_cursor;
	glm::vec3 normals[MAX_TILES];
	int normal_cursor;

	GLuint vao, vbuffer, tbuffer, nbuffer, tex[12], program;
	int tex_offset = 0;

	glm::mat4 global_rot = glm::mat4(1);
	int lookat = 0;
	GLfloat scale = 1;

	int cursorx_last, cursory_last, tile_last;
	bool global_rot_ctrl = false, local_rot_ctrl = false, animation_ready = true;

	int fpa = 15;
	const double PI = 3.1415926535897939;
}

using namespace glm;
using namespace rc;

void display();
void on_click(int, int, int, int);
void on_motion(int, int);
int get_clicked_tile(int, int);
void rot_layer(int, int, float);
void rot_layer(vec3, vec3, float);
void animation(vec3, vec3, int);
void on_scale(unsigned char, int, int);
void menu(int);
void dimension_menu(int dim_opt);
void tex_menu(int tex_opt);
void luminance_menu(int lum_opt);
void lookat_menu(int l_opt);
void background_menu(int bg_opt);

void push_tile(vec3 *tile) {
	for (int i = 0; i < 5; i++) {
		tiles[tile_cursor + i] = tile[i];
	}
	tile_cursor += 5;
}
void push_tex(vec2 *tex) {
	for (int i = 0; i < 5; i++) {
		textures[tex_cursor + i] = tex[i];
	}
	tex_cursor += 5;
}

bool floateq(double x, double y) {
	return abs(x - y) < 1e-5;
}

bool is01(double x) {
	return floateq(x, 0) || floateq(x, 1);
}

vec3 cross(vec3 v1, vec3 v2) {
	return vec3(v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z, v1.x * v2.y - v1.y * v2.x);
}

double dot(vec3 v1, vec3 v2) {
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}
// generate data for a n-D rubics cube;
void set_dim(int n) {
	dim = n;
	GLuint dim_loc = glGetUniformLocation(program, "dim");
	glUniform1i(dim_loc, dim);
	tile_cursor = 0;
	tex_cursor = 0;
	normal_cursor = 0;
	double tile_size = 0.5 / dim;
#define GEN_TILES_FOR_ONE_FACE(stmt) \
	for (int i = 0; i < dim; i++) { \
		for (int j = 0; j < dim; j++) { \
			double x = (i + 0.5) / dim, y = (j + 0.5) / dim; \
			stmt \
			vec2 tex[5] = { \
				vec2(x, y), \
				vec2(x + tile_size, y + tile_size), \
				vec2(x + tile_size, y - tile_size), \
				vec2(x - tile_size, y + tile_size), \
				vec2(x - tile_size, y - tile_size) \
									}; \
			push_tex(tex); \
		} \
	}

#define ONE_TILE_X(i) \
	vec3 tile[5] =  { \
		vec3(i, x, y), \
		vec3(i, x + tile_size, y + tile_size), \
		vec3(i, x + tile_size, y - tile_size), \
		vec3(i, x - tile_size, y + tile_size), \
		vec3(i, x - tile_size, y - tile_size) \
							}; \
	push_tile(tile); \
	for (int j = normal_cursor; j < normal_cursor + 5; j++) { \
		normals[j] = vec3(i == 0 ? -1 : 1, 0, 0); \
	} \
	normal_cursor += 5;

#define ONE_TILE_Y(i) \
	vec3 tile[5] =  { \
		vec3(x, i, y), \
		vec3(x + tile_size, i, y + tile_size), \
		vec3(x + tile_size, i, y - tile_size), \
		vec3(x - tile_size, i, y + tile_size), \
		vec3(x - tile_size, i, y - tile_size) \
				}; \
	push_tile(tile);\
	for (int j = normal_cursor; j < normal_cursor + 5; j++) { \
		normals[j] = vec3(0, i == 0 ? -1 : 1, 0); \
		} \
	normal_cursor += 5;

#define ONE_TILE_Z(i) \
	vec3 tile[5] =  { \
		vec3(x, y, i), \
		vec3(x + tile_size, y + tile_size, i), \
		vec3(x + tile_size, y - tile_size, i), \
		vec3(x - tile_size, y + tile_size, i), \
		vec3(x - tile_size, y - tile_size, i) \
			}; \
	push_tile(tile);\
	for (int j = normal_cursor; j < normal_cursor + 5; j++) { \
		normals[j] = vec3(0, 0, i == 0 ? -1 : 1); \
		} \
	normal_cursor += 5;


	GEN_TILES_FOR_ONE_FACE(ONE_TILE_X(0));
	GEN_TILES_FOR_ONE_FACE(ONE_TILE_X(1));
	GEN_TILES_FOR_ONE_FACE(ONE_TILE_Y(0));
	GEN_TILES_FOR_ONE_FACE(ONE_TILE_Y(1));
	GEN_TILES_FOR_ONE_FACE(ONE_TILE_Z(0));
	GEN_TILES_FOR_ONE_FACE(ONE_TILE_Z(1));

#undef GEN_TILES_FOR_ONE_FACE
#undef ONE_TILE_X

	glBindBuffer(GL_ARRAY_BUFFER, vbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(tiles), tiles, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, tbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(textures), textures, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, nbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(normals), normals, GL_STATIC_DRAW);
}

void init_tex() {
	tex[0] = loadTexture("red0.png");
	tex[1] = loadTexture("orange0.png");
	tex[2] = loadTexture("green0.png");
	tex[3] = loadTexture("blue0.png");
	tex[4] = loadTexture("white0.png");
	tex[5] = loadTexture("yellow0.png");
	tex[6] = loadTexture("red1.jpg");
	tex[7] = loadTexture("orange1.jpg");
	tex[8] = loadTexture("green1.jpg");
	tex[9] = loadTexture("blue1.jpg");
	tex[10] = loadTexture("white1.jpg");
	tex[11] = loadTexture("yellow1.jpg");
}

void init() {
	glewExperimental = true;
	if (glewInit() != GLEW_OK) {
		exit(0);
	}

	glClearColor(0, 0, 0.1, 0);
	GLuint vs = loadShader(GL_VERTEX_SHADER, "rc.vertex");
	GLuint fs = loadShader(GL_FRAGMENT_SHADER, "rc.fragment");
	program = linkProgram(std::vector < GLuint > {vs, fs});
	glUseProgram(program);

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbuffer);
	glGenBuffers(1, &tbuffer);
	glGenBuffers(1, &nbuffer);
	
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	
	set_dim(3);
	init_tex();
	setSampler("sampler", 0, program);

	glEnable(GL_DEPTH_TEST);

	glPointSize(10);

	glutDisplayFunc(display);
	glutIdleFunc(display);
	glutMouseFunc(on_click);
	glutMotionFunc(on_motion);
	glutKeyboardFunc(on_scale);

	
	int d_menu = glutCreateMenu(dimension_menu);
	glutAddMenuEntry("2", 2);
	glutAddMenuEntry("3", 3);
	glutAddMenuEntry("4", 4);
	glutAddMenuEntry("5", 5);

	int t_menu = glutCreateMenu(tex_menu);
	glutAddMenuEntry("Solid Colors", 0);
	glutAddMenuEntry("Impressionism", 1);

	int l_menu = glutCreateMenu(luminance_menu);
	glutAddMenuEntry("Direct Shading", 0);
	glutAddMenuEntry("Ambient & Directional", 1);
	glutAddMenuEntry("Only Directional", 2);
	glutAddMenuEntry("Point Light Source", 3);

	int lo_menu = glutCreateMenu(lookat_menu);
	glutAddMenuEntry("Othorgonal", 0);
	glutAddMenuEntry("Perspective", 1);
	
	int bg_menu = glutCreateMenu(background_menu);
	glutAddMenuEntry("Dark Blue", 0);
	glutAddMenuEntry("White", 1);
	glutAddMenuEntry("Black", 2);

	int main_menu = glutCreateMenu(menu);
	glutAddSubMenu("Dimension", d_menu);
	glutAddSubMenu("Texture", t_menu);
	glutAddSubMenu("Shading & Luminance", l_menu);
	glutAddSubMenu("Look At", lo_menu);
	glutAddSubMenu("Background", bg_menu);
	
	glutAttachMenu(GLUT_RIGHT_BUTTON);
}

void dimension_menu(int dim_opt) {
	set_dim(dim_opt);
}

void tex_menu(int tex_opt) {
	if (tex_opt == 0) {
		tex_offset = 0;
	}
	else if (tex_opt == 1){
		tex_offset = 6;
	}
}

void luminance_menu(int lum_opt) {
	GLuint shading_mod_loc = glGetUniformLocation(program, "shading_mode");
	glUniform1i(shading_mod_loc, lum_opt);
}

void lookat_menu(int l_opt) {
	GLuint lookat_loc = glGetUniformLocation(program, "lookat");
	glUniform1i(lookat_loc, l_opt);
}

void background_menu(int bg_opt) {
	if (bg_opt == 0) {
		glClearColor(0, 0, 0.1, 1);
	}
	else if (bg_opt == 1) {
		glClearColor(1, 1, 1, 1);
	}
	else if (bg_opt == 2) {
		glClearColor(0, 0, 0, 1);
	}
}

void display() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glBindBuffer(GL_ARRAY_BUFFER, vbuffer);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
	glBindBuffer(GL_ARRAY_BUFFER, tbuffer);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void *)0);
	glBindBuffer(GL_ARRAY_BUFFER, nbuffer);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
	for (int i = 0; i < 6; i++) {
		glBindTexture(GL_TEXTURE_2D, tex[tex_offset + i]);
		for (int j = 0; j < dim * dim; j++) 
			glDrawArrays(GL_TRIANGLE_STRIP, 1 + 5 * (i * dim * dim + j), 4);
	}

	glutSwapBuffers();
}

void on_click(int button, int state, int x, int y) {
	if (button == GLUT_LEFT_BUTTON) {
		if (state == GLUT_UP) {
			global_rot_ctrl = false;
			local_rot_ctrl = false;
		}
		else {
			int tid = get_clicked_tile(x, y);
			if (tid == 6 * dim * dim) {
				global_rot_ctrl = true;
			}
			else {
				local_rot_ctrl = true;
				tile_last = tid;
			}
		}
	}
	cursorx_last = x;
	cursory_last = y;
}

void on_motion(int x, int y) {
	double dx = x - cursorx_last;
	double dy = y - cursory_last;
	double cursor_move = dx * dx + dy * dy;

	if (global_rot_ctrl) {
		global_rot = glm::rotate(global_rot, (float)(5E-5 * cursor_move), glm::normalize(vec3(dy, dx, 0)));
		GLuint loc = glGetUniformLocation(program, "global_rot");
		glUniformMatrix4fv(loc, 1, GL_TRUE, (GLfloat*)&global_rot);
	}
	if (local_rot_ctrl) {
		int cur_tile = get_clicked_tile(x, y);
		if (cur_tile != tile_last && cur_tile != 6 * dim * dim && animation_ready) {
			local_rot_ctrl = false;
			global_rot_ctrl = false;
			animation_ready = false;
			animation(tiles[5 * tile_last], tiles[5 * cur_tile], 1);
			animation_ready = true;
		}
	}
	cursorx_last = x;
	cursory_last = y;
}

int get_clicked_tile(int x, int y) {
	if (!animation_ready) {
		return 6 * dim * dim;
	}
	GLuint query;
	glGenQueries(1, &query);
	glEnable(GL_SCISSOR_TEST);
	glScissor(x, 511 - y, 5, 5);
	int i;
	for (i = 0; i < 6 * dim * dim; i++) {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		glBeginQuery(GL_ANY_SAMPLES_PASSED, query);
			glDrawArrays(GL_TRIANGLE_STRIP, 1 + 5 * i, 4);
		glEndQuery(GL_ANY_SAMPLES_PASSED);

		GLint qready = 0;
		while (!qready) {
			glGetQueryObjectiv(query, GL_QUERY_RESULT_AVAILABLE, &qready);
		}

		GLint result;
		glGetQueryObjectiv(query, GL_QUERY_RESULT, &result);

		if (result) {
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			for (int j = 0; j < dim * dim * 6; j++) {
				if (i != j)
					glDrawArrays(GL_TRIANGLE_STRIP, 1 + 5 * j, 4);
			}
			glBeginQuery(GL_ANY_SAMPLES_PASSED, query);
				glDrawArrays(GL_TRIANGLE_STRIP, 1 + 5 * i, 4);
			glEndQuery(GL_ANY_SAMPLES_PASSED);
			qready = 0;
			while (!qready) {
				glGetQueryObjectiv(query, GL_QUERY_RESULT_AVAILABLE, &qready);
			}
			GLint front;
			glGetQueryObjectiv(query, GL_QUERY_RESULT, &front);
			if (front) {
				break;
			}
		}
	}
	glDeleteQueries(1, &query);
	glDisable(GL_SCISSOR_TEST);
	return i;
}

vec3 get_axis(int axis) {
	if (axis == 0) {
		return vec3(1, 0, 0);
	}
	else {
		return axis == 1 ? vec3(0, 1, 0) : vec3(0, 0, 1);
	}
}

void rot_layer(int axis, int layer, float angle) {
	for (int i = 0; i < 6 * dim * dim; i++) {
		if (min((int)(tiles[i * 5][axis] * dim), dim - 1) == layer) {
			mat4 rot = rotate(mat4(1), angle, get_axis(axis));
			for (int j = 0; j < 5; j++) {
				tiles[i * 5 + j] -= vec3(0.5, 0.5, 0.5);
				tiles[i * 5 + j] = (rot * vec4(tiles[i * 5 + j], 1)).xyz;
				tiles[i * 5 + j] += vec3(0.5, 0.5, 0.5);
				normals[i * 5 + j] = (rot * vec4(normals[i * 5 + j], 1)).xyz;
			}
		}
	}
	glBindBuffer(GL_ARRAY_BUFFER, vbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(tiles), tiles, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, nbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(normals), normals, GL_STATIC_DRAW);
}

void rot_layer(vec3 tile1, vec3 tile2, float angle) {
	if (floateq(tile1.x, tile2.x)) {
		if (!is01(tile1.x)) {
			double sgn = cross(tile1 - vec3(0.5, 0.5, 0.5), tile2 - vec3(0.5, 0.5, 0.5)).x > 0 ? 1 : -1;
			rot_layer(0, tile1.x * dim, sgn * angle);
			return;
		}
	}
	if (floateq(tile1.y, tile2.y)) {
		if (!is01(tile1.y)) {
			double sgn = cross(tile1 - vec3(0.5, 0.5, 0.5), tile2 - vec3(0.5, 0.5, 0.5)).y > 0 ? 1 : -1;
			rot_layer(1, tile1.y * dim, sgn * angle);
			return;
		}
	}
	if (floateq(tile1.z, tile2.z)) {
		if (!is01(tile1.z)) {
			double sgn = cross(tile1 - vec3(0.5, 0.5, 0.5), tile2 - vec3(0.5, 0.5, 0.5)).z > 0 ? 1 : -1;
			rot_layer(2, tile1.z * dim, sgn * angle);
			return;
		}
	}
}

void animation(vec3 tile1, vec3 tile2, int angle) {
	for (int i = 0; i < fpa; i++) {
		rot_layer(tile1, tile2, angle * (PI / fpa / 2.0));
		display();
	}
}

void on_scale(unsigned char key, int, int) {
	if (key == 'w') {
		rc::scale = min(rc::scale + 0.01, 1.14);
	}
	if (key == 's') {
		rc::scale = max(rc::scale - 0.01, 0.4);
	}
	if (key == 'a' && animation_ready) {
		fpa = max(10, fpa - 5);
	}
	if (key == 'd') {
		fpa = min(100, fpa + 5);
	}
	GLuint s_loc = glGetUniformLocation(program, "scale");
	glUniform1f(s_loc, rc::scale);
}

void menu(int) {}