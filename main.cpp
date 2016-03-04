#include "stdafx.h"

void init();

int main(int argc, char **argv) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
	glutInitWindowSize(512, 512);
	glutInitContextVersion(4, 1);
	glutInitContextProfile(GLUT_CORE_PROFILE);
	printf("!");
	
	
	glutCreateWindow("Hello");
	init();
	
	printf("%s\n", glGetString(GL_VENDOR));
	glutMainLoop();

	return 0;
}