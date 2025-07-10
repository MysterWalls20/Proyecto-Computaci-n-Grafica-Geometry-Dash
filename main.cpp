#ifdef __APPLE__
# include <OpenGL/gl.h>
# include <OpenGL/glu.h>
# include <GLUT/glut.h>
#else
# include <GL/gl.h>
# include <GL/glu.h>
# include <GL/glut.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


float jugador_y = 0.0f;               // Altura inicial del jugador
float velocidad = 0.0f;               // Velocidad vertical del jugador
int saltando = 0;                     // 1 si está saltando
int pausa = 0;                        // 1 si está pausado
const float gravedad = -1200.0f;      // Gravedad más fuerte por escala
const float salto = 950.0f;           // Velocidad inicial del salto
const int fps = 60;                   // Frames por segundo
float velocidad_obstaculo = 200.0f;   // Velocidad inicial del obstáculo (en unidades por segundo)
int puntaje = 0;
int record = 0;
int mostrar_texto_estado = 0;     // 1 para mostrar "PAUSA", 2 para mostrar "CONTINUA"
int contador_texto_estado = 0;    // contador para ocultar el texto de estado
int paso1 = 0, paso2 = 0, paso3 = 0;
int cambios_velocidad = 0;
int mostrar_gameover = 0;
int tiempo = 0;                       // Tiempo transcurrido en frames
float fondo_offset = 0.0f;            // Desplazamiento del fondo 

int fase = 1;


typedef struct {
	GLubyte *dibujo;
	GLuint bpp;
	GLuint largo;
	GLuint ancho;
	GLuint ID;
} textura;

textura tga_fondo, tga_jugador1, tga_gameover, tga_suelo,tga_fondo2,tga_suelo2,tga_jugador2;
textura tga_fondo3, tga_fondo4, tga_suelo3, tga_suelo4, tga_jugador3, tga_jugador4;

typedef struct {
	float x, y;
	int tipo;
	float ancho;
	float alto;
} Obstaculo;
Obstaculo obstaculos[] = {
	{600.0f, -135.0f, 1, 30.0f, 50.0f},
	{950.0f, -135.0f, 1, 30.0f, 50.0f},
	{1300.0f, -135.0f, 1, 30.0f, 50.0f},
	{1800.0f, 80.0f, 2, 40.0f, 100.0f},
	{2300.0f, 100.0f, 2, 40.0f, 100.0f},
	{2800.0f, 100.0f, 2, 40.0f, 100.0f},
	{3200.0f, -135.0f, 1, 30.0f, 80.0f},
	{3650.0f, -135.0f, 1, 30.0f, 80.0f},
	{4000.0f, -135.0f, 1, 30.0f, 80.0f}	
};
const int total_obstaculos = sizeof(obstaculos) / sizeof(obstaculos[0]);

int paso_obstaculo[9] = {0};
int punto_obstaculo[9] = {0};

float distancia_original[] = {
		obstaculos[1].x - obstaculos[0].x, 
		obstaculos[2].x - obstaculos[1].x,  
		obstaculos[3].x - obstaculos[2].x,  
		obstaculos[4].x - obstaculos[3].x,  
		obstaculos[5].x - obstaculos[4].x,  
		obstaculos[6].x - obstaculos[5].x,
		obstaculos[7].x - obstaculos[6].x,
		obstaculos[8].x - obstaculos[7].x,
		500.0f  
};
//carlos
int cargarTGA(const char *nombre, textura *imagen) {
	GLubyte cabezeraTGA[12] = {0,0,2,0,0,0,0,0,0,0,0,0};
	GLubyte compararTGA[12];
	GLubyte cabezera[6];
	GLuint bytesporpunto, tamanoimagen, temp, i;
	GLuint tipo = GL_RGBA;
	
	FILE *archivo = fopen(nombre, "rb");
	
	if (archivo == NULL || 
		fread(compararTGA, 1, sizeof(compararTGA), archivo) != sizeof(compararTGA) ||
		memcmp(cabezeraTGA, compararTGA, sizeof(compararTGA)) != 0 ||
		fread(cabezera, 1, sizeof(cabezera), archivo) != sizeof(cabezera)) {
		printf("No se encontr? o error en archivo %s\n", nombre);
		return 0;
	}
		
		imagen->largo = 256 * cabezera[1] + cabezera[0];
		imagen->ancho = 256 * cabezera[3] + cabezera[2];
		
		if (imagen->largo <= 0 || imagen->ancho <= 0 || (cabezera[4] != 24 && cabezera[4] != 32)) {
			printf("Datos inv?lidos en %s\n", nombre);
			fclose(archivo);
			return 0;
		}
		
		imagen->bpp = cabezera[4];
		bytesporpunto = cabezera[4] / 8;
		tamanoimagen = imagen->largo * imagen->ancho * bytesporpunto;
		
		imagen->dibujo = (GLubyte *) malloc(tamanoimagen);
		
		if (imagen->dibujo == NULL || fread(imagen->dibujo, 1, tamanoimagen, archivo) != tamanoimagen) {
			printf("Error leyendo %s\n", nombre);
			if (imagen->dibujo != NULL) free(imagen->dibujo);
			fclose(archivo);
			return 0;
		}
		
		for (i = 0; i < (int)tamanoimagen; i += bytesporpunto) {
			temp = imagen->dibujo[i];
			imagen->dibujo[i] = imagen->dibujo[i + 2];
			imagen->dibujo[i + 2] = temp;
		}
		
		fclose(archivo);
		
		glGenTextures(1, &imagen->ID);
		glBindTexture(GL_TEXTURE_2D, imagen->ID);
		
		if (imagen->bpp == 24) tipo = GL_RGB;
		
		glTexImage2D(GL_TEXTURE_2D, 0, tipo, imagen->ancho, imagen->largo, 0, tipo, GL_UNSIGNED_BYTE, imagen->dibujo);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		
		return 1;
}

//carlos
void cargando_texturas() {
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	if (!cargarTGA("tJugador.tga", &tga_jugador1)) printf("No se pudo cargar tJugador.tga\n");
	if (!cargarTGA("tSuelo.tga", &tga_suelo)) printf("No se pudo cargar tSuelo.tga\n");
	if (!cargarTGA("tJugador2.tga", &tga_jugador2)) printf("No se pudo cargar tJugador2.tga\n");
	if (!cargarTGA("tSuelo2.tga", &tga_suelo2)) printf("No se pudo cargar tSuelo2.tga\n");
	if (!cargarTGA("tFondo.tga", &tga_fondo)) printf("No se pudo cargar tFondo.tga\n");
	if (!cargarTGA("tGamerover.tga", &tga_gameover)) printf("No se pudo cargar tGamerover.tga\n");
	if (!cargarTGA("tFondo2.tga", &tga_fondo2)) printf("No se pudo cargar tFondo2.tga\n");
	if (!cargarTGA("tFondo3.tga", &tga_fondo3)) printf("No se pudo cargar tFondo3.tga\n");
	if (!cargarTGA("tFondo4.tga", &tga_fondo4)) printf("No se pudo cargar tFondo4.tga\n");
	if (!cargarTGA("tSuelo3.tga", &tga_suelo3)) printf("No se pudo cargar tSuelo3.tga\n");
	if (!cargarTGA("tSuelo4.tga", &tga_suelo4)) printf("No se pudo cargar tSuelo4.tga\n");
	if (!cargarTGA("tJugador3.tga", &tga_jugador3)) printf("No se pudo cargar tJugador3.tga\n");
	if (!cargarTGA("tJugador4.tga", &tga_jugador4)) printf("No se pudo cargar tJugador4.tga\n");
	
}
//frank
void reshape_cb(int w, int h) {
	if (h == 0) h = 1;
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glViewport(0, 0, w, h);
	glOrtho(-500.0, 500.0, -500.0, 500.0, -500.0, 500.0);
	glMatrixMode(GL_MODELVIEW);
}

//alonso
void luz(){
	GLfloat light_position[] = {0, 0, 1, 0}; 
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL); 
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	
}
	
	//gustavo
	void dibujar_fondo() {
		glLoadIdentity();
		glDisable(GL_LIGHTING);
		glEnable(GL_TEXTURE_2D);
		switch (fase) {
		case 1: glBindTexture(GL_TEXTURE_2D, tga_fondo.ID); break;
		case 2: glBindTexture(GL_TEXTURE_2D, tga_fondo2.ID); break;
		case 3: glBindTexture(GL_TEXTURE_2D, tga_fondo3.ID); break;
		case 4: glBindTexture(GL_TEXTURE_2D, tga_fondo4.ID); break;
		}
		
		
		glBegin(GL_QUADS);
		glTexCoord2f(0.0f + fondo_offset, 0.0f); glVertex2f(-500, 500);
		glTexCoord2f(1.0f + fondo_offset, 0.0f); glVertex2f(500, 500);
		glTexCoord2f(1.0f + fondo_offset, 1.0f); glVertex2f(500, -500);
		glTexCoord2f(0.0f + fondo_offset, 1.0f); glVertex2f(-500, -500);
		glEnd();
		
		glDisable(GL_TEXTURE_2D);
		glEnable(GL_LIGHTING);
	}
	
	//giordana
	void dibujar_suelo() {
		glLoadIdentity();
		glDisable(GL_LIGHTING);
		glEnable(GL_TEXTURE_2D);
		switch (fase) {
		case 1: glBindTexture(GL_TEXTURE_2D, tga_suelo.ID); break;
		case 2: glBindTexture(GL_TEXTURE_2D, tga_suelo2.ID); break;
		case 3: glBindTexture(GL_TEXTURE_2D, tga_suelo3.ID); break;
		case 4: glBindTexture(GL_TEXTURE_2D, tga_suelo4.ID); break;
		}
		glBegin(GL_QUADS);
		glTexCoord2f(0, 0); glVertex2f(-500, -135);  // parte superior del suelo
		glTexCoord2f(1, 0); glVertex2f(500, -135);  
		glTexCoord2f(1, 1); glVertex2f(500, -500);  // parte inferior del suelo
		glTexCoord2f(0, 1); glVertex2f(-500, -500);
		glEnd();
		glDisable(GL_TEXTURE_2D);
		glEnable(GL_LIGHTING);
	}
	
	//giordanna
	void dibujar_jugador() {
		glPushMatrix();
		glDisable(GL_LIGHTING);
		glEnable(GL_TEXTURE_2D);
		
		glColor3f(1, 1, 1);
		switch (fase) {
		case 1: glBindTexture(GL_TEXTURE_2D, tga_jugador1.ID); break;
		case 2: glBindTexture(GL_TEXTURE_2D, tga_jugador2.ID); break;
		case 3: glBindTexture(GL_TEXTURE_2D, tga_jugador3.ID); break;
		case 4: glBindTexture(GL_TEXTURE_2D, tga_jugador4.ID); break;
		}
		
		
		glTranslatef(-350.0f, jugador_y - 101.0f, 0); // Posición X fija, Y controlada por salto
		glScalef(50.0f, 50.0f, 1); // Tamaño del jugador: 100x100 unidades
		
		glBegin(GL_QUADS);
		glTexCoord2f(0,0); glVertex2f(-1, 1);
		glTexCoord2f(1,0); glVertex2f(1, 1); 
		glTexCoord2f(1,1); glVertex2f(1, -1);
		glTexCoord2f(0,1); glVertex2f(-1, -1);
		glEnd();
		
		
		glDisable(GL_TEXTURE_2D);
		glEnable(GL_LIGHTING);
		glPopMatrix();
	}
	
	//flavio
	// Función para dibujar cono
	void dibujar_cono(Obstaculo obs) {
		glDisable(GL_TEXTURE_2D);
		glEnable(GL_LIGHTING);
		glPushMatrix();
		glTranslatef(obs.x, obs.y, 0);
		
		switch (fase) {
		case 1: { // Amarillo
			GLfloat ambient[]  = {1.0f, 1.0f, 0.0f, 1.0f};
			GLfloat diffuse[]  = {1.0f, 1.0f, 0.0f, 1.0f};
			GLfloat specular[] = {1.0f, 1.0f, 0.4f, 1.0f};
			glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
			glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
			glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
			glMaterialf(GL_FRONT, GL_SHININESS, 50.0f);
			break;
		}
		case 2: { // Blanco con bordes celestes (nieve)
			GLfloat ambient[]  = {0.8f, 0.9f, 1.0f, 1.0f};
			GLfloat diffuse[]  = {1.0f, 1.0f, 1.0f, 1.0f};
			GLfloat specular[] = {0.6f, 0.8f, 1.0f, 1.0f};
			glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
			glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
			glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
			glMaterialf(GL_FRONT, GL_SHININESS, 80.0f);
			break;
		}
		case 3: { // Gris oscuro
			GLfloat ambient[]  = {0.2f, 0.2f, 0.2f, 1.0f};
			GLfloat diffuse[]  = {0.3f, 0.3f, 0.3f, 1.0f};
			GLfloat specular[] = {0.05f, 0.05f, 0.05f, 1.0f};
			glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
			glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
			glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
			glMaterialf(GL_FRONT, GL_SHININESS, 100.0f);
			break;
		}
		case 4: { // Marrón
			GLfloat ambient[]  = {0.4f, 0.2f, 0.0f, 1.0f};
			GLfloat diffuse[]  = {0.6f, 0.3f, 0.1f, 1.0f};
			GLfloat specular[] = {0.3f, 0.2f, 0.1f, 1.0f};
			glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
			glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
			glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
			glMaterialf(GL_FRONT, GL_SHININESS, 40.0f);
			break;
		}
		}
		
		glRotatef(-90, 1, 0, 0);
		glScalef(obs.ancho, 1.0f, obs.alto * 2);
		glutSolidCone(1.0, 1.0, 32, 32);
		
		glPopMatrix();
	}
	//flavio
	// Función para dibujar cubo
	void dibujar_cubo(Obstaculo obs) {
		glDisable(GL_TEXTURE_2D);
		glEnable(GL_LIGHTING);
		glPushMatrix();
		glTranslatef(obs.x, obs.y, 0);
		
		
		glRotatef(-10.0f, 1.0f, 0.0f, 0.0f);  // Rotación en X
		glRotatef(-10.0f, 0.0f, 1.0f, 0.0f);  // Rotación en Y
		switch (fase) {
		case 1: { // Rojo
			GLfloat ambient[]  = {0.3f, 0.0f, 0.0f, 1.0f};
			GLfloat diffuse[]  = {1.0f, 0.0f, 0.0f, 1.0f};
			GLfloat specular[] = {0.5f, 0.0f, 0.0f, 1.0f};
			glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
			glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
			glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
			glMaterialf(GL_FRONT, GL_SHININESS, 30.0f);
			break;
		}
		case 2: { // Blanco con bordes celestes (nieve)
			GLfloat ambient[]  = {0.8f, 0.9f, 1.0f, 1.0f};
			GLfloat diffuse[]  = {1.0f, 1.0f, 1.0f, 1.0f};
			GLfloat specular[] = {0.6f, 0.8f, 1.0f, 1.0f};
			glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
			glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
			glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
			glMaterialf(GL_FRONT, GL_SHININESS, 80.0f);
			break;
		}
		case 3: { // Negro con bordes rojos
			GLfloat ambient[]  = {0.1f, 0.0f, 0.0f, 1.0f};
			GLfloat diffuse[]  = {0.2f, 0.0f, 0.0f, 1.0f};
			GLfloat specular[] = {0.8f, 0.0f, 0.0f, 1.0f};
			glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
			glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
			glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
			glMaterialf(GL_FRONT, GL_SHININESS, 100.0f);
			break;
		}
		case 4: { // Morado
			GLfloat ambient[]  = {0.3f, 0.0f, 0.3f, 1.0f};
			GLfloat diffuse[]  = {0.6f, 0.0f, 0.6f, 1.0f};
			GLfloat specular[] = {0.9f, 0.3f, 0.9f, 1.0f};
			glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
			glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
			glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
			glMaterialf(GL_FRONT, GL_SHININESS, 60.0f);
			break;
		}
		}
		glutSolidCube(obs.ancho * 2);
		
		glPopMatrix();
	}
	
	//flavio
	// Nueva función que selecciona qué dibujar según el tipo
	void dibujar_obstaculo_3d(Obstaculo obs) {
		switch (obs.tipo) {
		case 1:
			dibujar_cono(obs);
			break;
		case 2:
			dibujar_cubo(obs);
			break;
		default:
			printf("Tipo de obstaculo desconocido\n");
			break;
		}
	}
	
	//gustavo
	void mostrar_gameover_imagen() {
		if (!mostrar_gameover) return;
		
		
		glLoadIdentity();
		glDisable(GL_LIGHTING);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, tga_gameover.ID);
		
		glBegin(GL_QUADS);
		glTexCoord2f(0, 0); glVertex2f(-500, 500);
		glTexCoord2f(1, 0); glVertex2f(500, 500);
		glTexCoord2f(1, 1); glVertex2f(500, -500);
		glTexCoord2f(0, 1); glVertex2f(-500, -500);
		glEnd();
		
		glDisable(GL_TEXTURE_2D);
		glEnable(GL_LIGHTING);
	}
	//alonso
	int esta_sobre_cubo(Obstaculo obs) {
		float factor_colision_cubo = 0.8f; // debe coincidir con el usado en colision()
		
		float lado_visual = obs.ancho * 2.0f;
		float lado_colision = lado_visual * factor_colision_cubo;
		
		float obst_izq = obs.x - lado_colision / 2.0f;
		float obst_der = obs.x + lado_colision / 2.0f;
		float obst_arr = obs.y + lado_colision / 2.0f;
		
		float jugador_x = -350.0f;
		float jugador_y_real = jugador_y - 101.0f;
		float jugador_ancho = 50.0f;
		float jugador_aba = jugador_y_real - jugador_ancho;
		
		return (jugador_y > 0 &&
				jugador_x + jugador_ancho > obst_izq &&
				jugador_x - jugador_ancho < obst_der &&
				fabs(jugador_aba - obst_arr) <= 20.0f &&
				velocidad <= 0);
	}
	//alonso
	int colision(Obstaculo obs) {
		// Posición y tamaño del jugador
		float jugador_x = -350.0f;
		float jugador_y_real = jugador_y - 101.0f;
		float jugador_ancho = 50.0f;
		float jugador_alto = 50.0f;
		
		float jugador_izq = jugador_x - jugador_ancho;
		float jugador_der = jugador_x + jugador_ancho;
		float jugador_arr = jugador_y_real + jugador_alto;
		float jugador_aba = jugador_y_real - jugador_alto;
		
		// Tamaño real del obstáculo basado en su tipo y escala visual
		float obst_izq, obst_der, obst_arr, obst_aba;
		
		if (obs.tipo == 1) { 
			float factor_base_real = 0.5f; 
			float radio_base = obs.ancho * factor_base_real;
			float altura = obs.alto * 1.8;
			
			obst_izq = obs.x - radio_base;
			obst_der = obs.x + radio_base-1.0;
			obst_aba = obs.y;
			obst_arr = obs.y + altura;
		}
		else if (obs.tipo == 2) {
			float factor_colision_cubo = 0.8f;
			float lado_visual = obs.ancho * 2.0f; // lado del cubo dibujado
			float lado_colision = lado_visual * factor_colision_cubo; // bounding box ajustado
			
			obst_izq = obs.x - lado_colision / 2.0f;
			obst_der = obs.x + lado_colision / 2.0f;
			obst_aba = obs.y - lado_colision / 2.0f;
			obst_arr = obs.y + lado_colision / 2.0f;
			
			if (esta_sobre_cubo(obs)) return 0;
		}
		else return 0; // tipo desconocido
		
		// Colisión AABB
		if (jugador_der < obst_izq) return 0;
		if (jugador_izq > obst_der) return 0;
		if (jugador_arr < obst_aba) return 0;
		if (jugador_aba > obst_arr) return 0;
		
		return 1;
	}
	//alonso
	
	
	//carlos
	void dibujar_texto(float x, float y, const char* texto, void* fuente) {
		glRasterPos2f(x, y);
		while (*texto) {
			glutBitmapCharacter(fuente, *texto);
			texto++;
		}
	}	
	//carlos
	void mostrar_puntaje() {
		glDisable (GL_LIGHTING);
		glColor3f(1.0f, 1.0f, 1.0f);  // Blanco
		
		char texto[64];
		sprintf(texto, "Puntaje: %06d  Record: %06d", puntaje, record);
		
		dibujar_texto(-490.0f, -490.0f, texto, GLUT_BITMAP_HELVETICA_12);
		glEnable(GL_LIGHTING);
	}
	//frank
	void mostrar_estado_pausa() {
		if (mostrar_texto_estado == 0) return;
		
		glDisable(GL_LIGHTING);
		glDisable(GL_TEXTURE_2D);
		glColor3f(1.0f, 1.0f, 0.4f);  
		
		if (mostrar_texto_estado == 1)
			dibujar_texto(-70.0f, 0.0f, "PAUSA", GLUT_BITMAP_TIMES_ROMAN_24);
		else if (mostrar_texto_estado == 2)
			dibujar_texto(-100.0f, 0.0f, "CONTINUA", GLUT_BITMAP_TIMES_ROMAN_24);
		
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_LIGHTING);
	}
	//alonso
	void display_cb() {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glLoadIdentity();
		
		dibujar_fondo();
		dibujar_suelo();
		mostrar_puntaje();
		
		if (mostrar_gameover) {
			mostrar_gameover_imagen();  // Solo muestra la imagen final
		} else {
			dibujar_jugador();
			for (int i = 0; i < total_obstaculos; i++) {
				dibujar_obstaculo_3d(obstaculos[i]);
			}
		}
		
		
		
		//glPushAttrib(GL_ALL_ATTRIB_BITS);
		mostrar_estado_pausa();
		//glPopAttrib();
		
		
		glutSwapBuffers();
	}
	
	//alexis
	
	void update(int value) {
		if (!pausa) {
			float jugador_ancho = 50.0f;
			float jugador_alto = 50.0f;
			// Movimiento vertical del jugador (salto)
			if (saltando || jugador_y > 0) {
				// Aumentar la gravedad si está cayendo
				if (velocidad < 0) {
					float factor_gravedad = 6.0f + (velocidad_obstaculo - 200.0f) / 100.0f;
					if (factor_gravedad > 25.0f) factor_gravedad = 25.0f;  // Límite superior opcional
					velocidad += (gravedad * factor_gravedad) / fps;
				} else {
					velocidad += gravedad / fps;  // Salto normal
				}
				jugador_y += velocidad / fps;
				// Verificar si el jugador está encima de un cubo (como plataforma)
				int sobre_cubo = 0;
				for (int i = 0; i < total_obstaculos; i++) {
					if (obstaculos[i].tipo == 2 && esta_sobre_cubo(obstaculos[i])) {
						float altura_cubo = obstaculos[i].y + obstaculos[i].alto;
						jugador_y = altura_cubo + 0.5f + 81.5f;
						velocidad = 0.0f;
						saltando = 0;
						
						if (punto_obstaculo[i] == 0) {
							puntaje += 10;
							punto_obstaculo[i] = 1;
						}
						sobre_cubo = 1;
						break;
					}
				}
				
				if (jugador_y <= 0.0f && !sobre_cubo) {
					jugador_y = 0.0f;
					velocidad = 0.0f;
					saltando = 0;
				}
			}
			
			for (int i = 0; i < total_obstaculos; i++) {
				obstaculos[i].x -= velocidad_obstaculo / fps;
				
				if (obstaculos[i].x < -600.0f) {
					if (i == 0)
						obstaculos[i].x = obstaculos[total_obstaculos - 1].x + distancia_original[total_obstaculos - 1];
					else
						obstaculos[i].x = obstaculos[i - 1].x + distancia_original[i - 1];
					
					paso_obstaculo[i] = 0;
					punto_obstaculo[i] = 0;
				}
				
			}
			for (int i = 0; i < total_obstaculos; i++) {
				if (obstaculos[i].tipo == 1) {  // solo tipo 1 (cono) se puntúa por pasarlo
					if (obstaculos[i].x + obstaculos[i].ancho < -350.0f && paso_obstaculo[i] == 0) {
						puntaje += 10;
						paso_obstaculo[i] = 1;
					}
				}
			}
			
			
			// Aumentar la velocidad cada cierto tiempo
			tiempo++;
			if (tiempo % (15 * fps) == 0) {
				velocidad_obstaculo += 50.0f;
				cambios_velocidad++;
				
				fase++;
				if (fase > 4) fase = 1;  // volver a la fase 1
				printf("Fase %d activada\n", fase);
			}
			
			for (int i = 0; i < total_obstaculos; i++) {
				if (colision(obstaculos[i])) {
					printf("¡Perdiste!\n");
					if (puntaje > record) record = puntaje;
					pausa = 1;
					mostrar_gameover = 1;
					break;  // Terminar el bucle tras detectar la colisión
				}
			}
				
				// Fondo que se desplaza
				fondo_offset += (velocidad_obstaculo / fps) * 0.001f;  // movimiento proporcional
				if (fondo_offset > 1.0f) fondo_offset -= 1.0f;
		}
		if (mostrar_texto_estado == 2 && contador_texto_estado > 0) {
			contador_texto_estado--;
			if (contador_texto_estado == 0)
				mostrar_texto_estado = 0;
		}
		glutPostRedisplay();
		glutTimerFunc(1000 / fps, update, 0);
	}
	
	//alexis
	void keyboard_cb(unsigned char key, int x, int y) {
		if (mostrar_gameover) {
			switch (key) {
			case 'r':
			case 'R': {
				jugador_y = 0;
				saltando = 0;
				velocidad = 0;
				pausa = 0;
				tiempo = 0;
				velocidad_obstaculo = 200.0f;
				puntaje = 0;
				mostrar_gameover = 0;
				
				float posiciones_iniciales[] = {600, 950, 1300, 1700, 2200, 2700,3200,3650,4000};
				for (int i = 0; i < total_obstaculos; i++) {
					obstaculos[i].x = posiciones_iniciales[i];
					paso_obstaculo[i] = 0;
					punto_obstaculo[i] = 0;
				}
				
				cambios_velocidad = 0;
				fase = 1;
				break;
			}
			case 'x':
			case 'X':
				exit(0);
				break;
			}
		} else {
			switch (key) {
			case 13:  // Enter
				pausa = !pausa;
				if (pausa)
					mostrar_texto_estado = 1;  // Mostrar "PAUSA"
				else {
					mostrar_texto_estado = 2;  // Mostrar "CONTINUA"
					contador_texto_estado = fps * 0.5;  // Mostrarlo por 2 segundos
				}
				break;
				
			case ' ':
				if (!saltando && jugador_y == 0 && !pausa) {
					saltando = 1;
					velocidad = salto;
				}
				break;
			}
		}
	}
	
	void initialize() {
		glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
		glutInitWindowSize(800, 600);
		glutInitWindowPosition (400,20);
		glutCreateWindow("EF");
		cargando_texturas();
		luz();
		
		glutDisplayFunc(display_cb);
		glutReshapeFunc(reshape_cb);
		
		glutTimerFunc(1000 / fps, update, 0);
		glutKeyboardFunc(keyboard_cb);
		glClearColor(0, 0, 0, 1);
	}
	
	int main(int argc, char **argv) {
		glutInit(&argc, argv);
		initialize();
		glutMainLoop();
		return 0;
	}
	
	
	
