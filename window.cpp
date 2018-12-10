#include <GL4D/gl4du.h>
#include <GL4D/gl4dg.h>
#include <GL4D/gl4duw_SDL2.h>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <opencv2/objdetect.hpp>

using namespace cv;
using namespace std;

/* Prototypes des fonctions statiques contenues dans ce fichier C */
static void init(void);
static void resize(int w, int h);
static void draw(void);
static void quit(void);
/*!\brief dimensions de la fen�tre */
static int _w = 640, _h = 480;
/*!\brief identifiant du quadrilat�re */
static GLuint _quad = 0, _sphere = 0;
/*!\brief identifiants des (futurs) GLSL programs */
static GLuint _pId = 0;
/*!\brief identifiant de la texture charg�e */
static GLuint _tId = 0;
/*!\brief device de capture OpenCV */

GLfloat color[4] = {0, 1 , 0 , 1};
int formChoice = 0;
/* essayer d'ouvrir la seconde cam�ra */

// Si la source est une vid�o :
  //VideoCapture _cap("./assets/discours.mp4");
// Si la source est une cam�ra :
  VideoCapture _cap(1);

CascadeClassifier * face_cc = NULL;
CascadeClassifier * eye_cc = NULL;

/*!\brief Change les paramètres lors de l'évenement key down.*/
void interactivity(int keycode){
    printf("key code %d", keycode );
    switch (keycode) {
        case 97:
            color[0] = (color[0] == 0) ? 1 : 0;
        break;

        case 122:
            formChoice = (formChoice == 1) ? 0 : 1;
         break;
    }
}


/*!\brief Cr�ation de la fen�tre et param�trage des fonctions callback.*/
int main(int argc, char ** argv) {
  /* si echec */

    // Si la source est une vid�o :
    /*if(!_cap.isOpened()){
      printf("Video is not opened app crashed !! \n");
      return 1;
    }*/
    // Si la source est une cam�ra :

  if(!_cap.isOpened()) {
    _cap.open(CV_CAP_ANY);
    if(!_cap.isOpened())
      return 1;
  }

  face_cc = new CascadeClassifier("haarcascade_frontalface_default.xml");
  eye_cc = new CascadeClassifier("haarcascade_eye.xml");
  if(face_cc == NULL || eye_cc == NULL)
    return 2;

  /* on modifie les dimensions de capture */
  _cap.set(CV_CAP_PROP_FRAME_WIDTH,  640);
  _cap.set(CV_CAP_PROP_FRAME_HEIGHT, 480);
  /* on r�cup�re et imprime les dimensions de la vid�o */
  Size s(_cap.get(CV_CAP_PROP_FRAME_WIDTH), _cap.get(CV_CAP_PROP_FRAME_HEIGHT));
  _w = (int)s.width;
  _h = (int)s.height;
  if(!gl4duwCreateWindow(argc, argv, "GL4Dummies", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
			 _w, _h, SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN))
    return 1;
  
  init();
  atexit(quit);
  gl4duwResizeFunc(resize);
  gl4duwDisplayFunc(draw);
  gl4duwMainLoop();

  return 0;
}

/*!\brief Initialise les param�tres OpenGL.*/
static void init(void) {
  _pId = gl4duCreateProgram("<vs>shaders/basic.vs", "<fs>shaders/basic.fs", NULL);
  glClearColor(0.2f, 0.2f, 0.2f, 0.0f);

  gl4duGenMatrix(GL_FLOAT, "modelView");
  gl4duGenMatrix(GL_FLOAT, "projection");

  resize(_w, _h);
  _quad = gl4dgGenQuadf();
  _sphere = gl4dgGenSpheref(10, 10);
  glGenTextures(1, &_tId);
  glBindTexture(GL_TEXTURE_2D, _tId);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  GLuint p = 255<<16;
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_BGRA, GL_UNSIGNED_BYTE, &p);
}


/*!\brief Param�trage de la vue (viewPort) OpenGL en fonction
 * des dimensions de la fen�tre.
 * \param w largeur de la fen�tre transmise par GL4Dummies.
 * \param h hauteur de la fen�tre transmise par GL4Dummies.
 */
static void resize(int w, int h) {
  _w  = w; _h = h;
  glViewport(0, 0, _w, _h);
  gl4duBindMatrix("projection");
  gl4duLoadIdentityf();
  gl4duFrustumf(-0.5, 0.5, -0.5, 0.5, 1, 1000);
  gl4duBindMatrix("modelView");
}


/*!\brief Dessin de la g�om�trie textur�e. */
static void draw(void) {
  static GLfloat dx = 0.0f;
  GLfloat rect[4] = {0, 0, 0, 0};
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glUseProgram(_pId);
  glEnable(GL_DEPTH_TEST);
  gl4duBindMatrix("modelView");
  gl4duLoadIdentityf();
  Mat frame, gsi;
  /* r�cup�rer la frame courante */
  _cap >> frame;
  cvtColor(frame, gsi, COLOR_BGR2GRAY);
  vector<Rect> faces;
  face_cc->detectMultiScale(gsi, faces, 1.1, 5);

  gl4duwKeyDownFunc(&interactivity); //Interactivity

  for (vector<Rect>::iterator fc = faces.begin(); fc != faces.end(); ++fc) {
    rect[0] = ((*fc).x - _w / 2.0f) / (_w / 2.0f);
    rect[1] = (_h / 2.0f - (*fc).y) / (_h / 2.0f);
    rect[2] = (*fc).width / (GLfloat)_w;
    rect[3] = (*fc).height / (GLfloat)_h;

    glUniform4fv(glGetUniformLocation(_pId, "color"), 1, color);
    glUniform1i(glGetUniformLocation(_pId, "test"), 0);
    
    gl4duPushMatrix();
    gl4duTranslatef(rect[0] + rect[2], rect[1] - rect[3], -1.8);
    gl4duScalef(rect[2], rect[3], 0.2);
    //gl4duRotatef(-dx * 1000, 0, 0, 1);
    gl4duSendMatrices();
    gl4duPopMatrix();
    //d�cal�e r�duite

    //Change la forme en fonction de la touche enfoncée
      if(formChoice == 0) {
          gl4dgDraw(_quad);
      } else {
          gl4dgDraw(_sphere);
      }
  }
  glBindTexture(GL_TEXTURE_2D, _tId);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, frame.cols, frame.rows, 0, GL_BGR, GL_UNSIGNED_BYTE, frame.ptr());
  glUniform1i(glGetUniformLocation(_pId, "inv"), 1);
  glUniform1i(glGetUniformLocation(_pId, "myTexture"), 0);

  gl4duTranslatef(0, 0, -2);
  gl4duSendMatrices();
  glUniform1i(glGetUniformLocation(_pId, "test"), 1);

  //full centr�e
  gl4dgDraw(_quad);
  
  dx -= 0.01f;

}


/*!\brief Appel�e au moment de sortir du programme (atexit). Elle
 *  lib�re les donn�es utilis�es par OpenGL et GL4Dummies.*/
static void quit(void) {
  if(_tId)
    glDeleteTextures(1, &_tId);
  gl4duClean(GL4DU_ALL);
}
