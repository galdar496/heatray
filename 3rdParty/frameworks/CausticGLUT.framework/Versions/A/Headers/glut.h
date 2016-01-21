#ifndef CAUSTIC_GLUT_H
#define CAUSTIC_GLUT_H 1

#include <OpenRL/rl.h>

#if !defined(GLUTAPI)
#  if defined(_WIN32)
#    define GLUTAPI __declspec(dllimport)
#  else
#    define GLUTAPI
#  endif
#endif

/*
 * Display modes
 */
#define  GLUT_RGBA                          0
#define  GLUT_DOUBLE                        2

/*
 * Caustic extension for 128-bpp framebuffer
 */
#define  GLUT_FLOAT_CST						0x1000

/*
 * Mouse buttons
 */
#define  GLUT_LEFT_BUTTON					0
#define  GLUT_MIDDLE_BUTTON					1
#define  GLUT_RIGHT_BUTTON					2
#define  GLUT_DOWN							0
#define  GLUT_UP							1

#ifdef __cplusplus
extern "C" {
#endif

void GLUTAPI glutInit(int *argcp, char **argv);
void GLUTAPI glutInitDisplayMode(unsigned int mode);
void GLUTAPI glutShutdown(void);

void GLUTAPI glutInitWindowPosition(int x, int y);
void GLUTAPI glutInitWindowSize(int width, int height);
void GLUTAPI glutMainLoop(void);

int  GLUTAPI glutCreateWindow(const char *title);
void GLUTAPI glutPostRedisplay(void);
	
void GLUTAPI glutReshapeWindow(int width, int height);

void GLUTAPI glutSwapBuffers(void);

void GLUTAPI glutDisplayFunc(void (*func)(void));
void GLUTAPI glutReshapeFunc(void (*func)(int width, int height));
void GLUTAPI glutKeyboardFunc(void (*func)(unsigned char key, int x, int y));
void GLUTAPI glutKeyboardUpFunc(void (*func)( unsigned char key, int x, int y ));
void GLUTAPI glutIdleFunc(void (*func)(void));
void GLUTAPI glutShutdownFunc(void (*func)(void));
void GLUTAPI glutMouseFunc(void (*func)(int button, int state, int x, int y));
void GLUTAPI glutMotionFunc(void (*func)(int x, int y));
void GLUTAPI glutPassiveMotionFunc(void (*func)(int x, int y));

void GLUTAPI glutSpaceballMotionFunc(void (*func)(int x, int y, int z));
void GLUTAPI glutSpaceballRotateFunc(void (*func)(int x, int y, int z));

#ifdef __cplusplus
}
#endif

#endif /* CAUSTIC_GLUT_H */

