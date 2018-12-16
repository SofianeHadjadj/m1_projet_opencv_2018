#include <GL4D/gl4du.h>
#include <GL4D/gl4dg.h>
#include <GL4D/gl4duw_SDL2.h>
#include <GL4D/gl4df.h>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <opencv2/objdetect.hpp>
#include <SDL_image.h>


using namespace cv;
using namespace std;

/* Prototypes des fonctions statiques contenues dans ce fichier C */
static void init(void);
static void resize(int w, int h);
static void draw(void);
static void quit(void);
static void interactivity(int keycode);
static void changeFilter(void);
static void saveScreenshotToFile(std::string filename, int windowWidth, int windowHeight);


/*!\brief dimensions de la fen�tre */
static int _w = 640, _h = 480;

/*!\brief identifiant du quadrilat�re */
static GLuint _quad = 0, _sphere = 0;

/*!\brief identifiants des (futurs) GLSL programs */
static GLuint _pId = 0;

/*!\brief identifiant de la texture charg�e */
static GLuint _tId = 0;
static GLuint _eId = 0;

/*!\brief Les choix de l'utilisateur */
static int formChoice = -1;
static int filterChoice = -1;

/*!\brief Les texture à charger */
const char* _texture_filenames[] = {"assets/masque.png", "assets/nez.png", "assets/lunettes.png"};
static GLuint _mId[3] = {0};

/*!\brief device de capture OpenCV */
VideoCapture _cap(1);

/*!\brief Haar classifier */
CascadeClassifier * face_cc = NULL;
CascadeClassifier * right_eye_cc = NULL;
CascadeClassifier * left_eye_cc = NULL;


/*!\brief Cr�ation de la fen�tre et param�trage des fonctions callback.*/
int main(int argc, char ** argv) {

    // Ouverture de la caméra
    if(!_cap.isOpened()) {
    _cap.open(CV_CAP_ANY);
    if(!_cap.isOpened())
        return 1;
    }

    face_cc = new CascadeClassifier("haarcascade_frontalface_default.xml");
    right_eye_cc = new CascadeClassifier("ojoI.xml");
    left_eye_cc = new CascadeClassifier("ojoD.xml");

    if(face_cc == NULL || right_eye_cc == NULL)
    return 2;

    // on modifie les dimensions de capture
    _cap.set(CV_CAP_PROP_FRAME_WIDTH,  640);
    _cap.set(CV_CAP_PROP_FRAME_HEIGHT, 480);

    // on récupere et imprime les dimensions de la vidéo
    Size s(_cap.get(CV_CAP_PROP_FRAME_WIDTH), _cap.get(CV_CAP_PROP_FRAME_HEIGHT));
    _w = (int)s.width;
    _h = (int)s.height;

    if(!gl4duwCreateWindow(argc, argv, "GL4Dummies", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, _w, _h, SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN))
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

    gl4duGenMatrix(GL_FLOAT, "modelView"); // Matrice pour le placement des objets et de la caméra
    gl4duGenMatrix(GL_FLOAT, "projection"); // Projection de l'espace en 2D

    resize(_w, _h);
    _quad = gl4dgGenQuadf();
    _sphere = gl4dgGenSpheref(10, 10);


    // Création des textures

    // Ecran
    glGenTextures(1, &_tId);
    glBindTexture(GL_TEXTURE_2D, _tId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    GLuint p = 255<<16;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_BGRA, GL_UNSIGNED_BYTE, &p);

    // Elements superposés sur la video
    int i;
    for (i = 0; i < 3; i++) {
    glGenTextures(1, &_mId[i]);
    SDL_Surface *t;
    glBindTexture(GL_TEXTURE_2D, _mId[i]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    if( (t = IMG_Load(_texture_filenames[i])) != NULL ) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, t->w, t->h, 0, GL_BGRA, GL_UNSIGNED_BYTE, t->pixels);
        SDL_FreeSurface(t);
        } else
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
    }

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
    gl4duLoadIdentityf(); // chargement de la matrice
    gl4duFrustumf(-0.5, 0.5, -0.5, 0.5, 1, 1000);
    gl4duBindMatrix("modelView");
}

int t = 0;
/*!\brief Dessin de la g�om�trie textur�e. */
static void draw(void) {

    static GLfloat dx = 0.0f;
    GLfloat rect[4] = {0, 0, 0, 0};
    GLfloat rectEyes[4] = {0, 0, 0, 0};
    // Efface le buffer de couleur et de profondeur.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(_pId);
    glEnable(GL_DEPTH_TEST);
    // Gestion de la transparence
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Mat frame, gsi;
    // Récuperer la frame courante
    _cap >> frame;

    // Ajout de la texture pour la video
    glBindTexture(GL_TEXTURE_2D, _tId); // bind a named texture to a texturing target
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, frame.cols, frame.rows, 0, GL_BGR, GL_UNSIGNED_BYTE, frame.ptr()); // specify a two-dimensional texture image
    glUniform1i(glGetUniformLocation(_pId, "inv"), 1);
    glUniform1i(glGetUniformLocation(_pId, "myTexture"), 0);
    gl4duTranslatef(0, 0, -2);
    gl4duSendMatrices();
    glUniform1i(glGetUniformLocation(_pId, "test"), 1);
    gl4dgDraw(_quad);

    gl4duLoadIdentityf(); //glLoadIdentity replaces the current matrix with the identity matrix

    // Ajout de texture sur les visages
    if(formChoice != -1) {

        cvtColor(frame, gsi, COLOR_BGR2GRAY); //GSI est l'image sur laquelle on va rechercher les visages
        vector <Rect> faces;
        face_cc->detectMultiScale(gsi, faces, 1.1, 5); // les deux derniers paramètres correspondent au degré de précision

        for (vector<Rect>::iterator fc = faces.begin(); fc != faces.end(); ++fc) {
            rect[0] = ((*fc).x - _w / 2.0f) / (_w / 2.0f);
            rect[1] = (_h / 2.0f - (*fc).y) / (_h / 2.0f);
            rect[2] = (*fc).width / (GLfloat) _w;
            rect[3] = (*fc).height / (GLfloat) _h;

            glBindTexture(GL_TEXTURE_2D, _mId[formChoice]);
            glUniform1i(glGetUniformLocation(_pId, "masque"), 0); // Le dernier paramètre, le niveau de la texture
            gl4duPushMatrix(); // Empile (sauvegarde) la matrice courante
            gl4duTranslatef(rect[0] + rect[2], rect[1] - rect[3], -1.5);
            gl4duScalef(rect[2], rect[3], 0.2);
            //gl4duRotatef(-dx * 1000, 0, 0, 1);
            gl4duSendMatrices(); // Envoie toutes matrices au program shader en cours et en utilisant leurs noms pour obtenir le uniform location
            gl4duPopMatrix(); // Empile la matrice courante en restaurant l'état précédemment sauvegardé

            gl4dgDraw(_quad);

        }
    }

    // Interactivite
    gl4duwKeyDownFunc(&interactivity);

    // Filtre sobel
    if(filterChoice != -1){
        changeFilter();
        gl4dgDraw(_quad);
    }

}

///*!\brief Screenshot sous openGL thanks : https://stackoverflow.com/questions/5844858/how-to-take-screenshot-in-opengl*/
void saveScreenshotToFile(std::string filename, int windowWidth, int windowHeight) {
    const int numberOfPixels = windowWidth * windowHeight * 3;
    unsigned char pixels[numberOfPixels];
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer(GL_FRONT);
    glReadPixels(0, 0, windowWidth, windowHeight, GL_BGR_EXT, GL_UNSIGNED_BYTE, pixels);
    FILE *outputFile = fopen(filename.c_str(), "w");
    short header[] = {0, 2, 0, 0, 0, 0, (short) windowWidth, (short) windowHeight, 24};
    fwrite(&header, sizeof(header), 1, outputFile);
    fwrite(pixels, numberOfPixels, 1, outputFile);
    fclose(outputFile);
    printf("Finish writing to file.\n");
}

///*!\brief Change les paramètres lors de l'évenement key down.*/
void interactivity(int keycode){
    printf("key code %d", keycode );
    switch (keycode) {
        case 97: //a
            filterChoice = -1;
            formChoice = (formChoice == 2) ? 0 : formChoice + 1;
            break;
        case 122: //z
            formChoice = -1;
            filterChoice = (filterChoice == 1 ) ? 0 : filterChoice + 1;
            break;
        case 101: //z
            saveScreenshotToFile("screenshot.tga" + std::to_string(rand() % 100), _w, _h);
            break;
  }
}

///*!\brief Appel un filtre different en fonction du choix utilisateur*/
static void changeFilter(void) {
    switch(filterChoice){
        case 0:
            gl4dfSobel(_tId, 0, true);
            break;
        case 1:
            gl4dfBlur(_tId, 0, 10, 1, 0, 1);
            break;
    }
}


/*!\brief Appel�e au moment de sortir du programme (atexit). Elle
 *  lib�re les donn�es utilis�es par OpenGL et GL4Dummies.*/
static void quit(void) {
  if(_tId)
    glDeleteTextures(1, &_tId);
  gl4duClean(GL4DU_ALL);
}

