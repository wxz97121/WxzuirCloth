#include "include\GL\freeglut.h"
#include <cmath>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <string>
#include "Vec3.h"
const double DAMPING = 0.0005;
const double TIME_STEPSIZE = 0.25;
const double TIME_STEPSIZE2 = TIME_STEPSIZE*TIME_STEPSIZE;
const int LIENS_ITERATIONS = 1;
const bool isVerlet = false;
using namespace std;



// 单个的点
class Particule {
private:
	bool movable; // 是否可以移动
	float mass; // 质量
	Vec3 pos; // 位置
	Vec3 old_pos;
	Vec3 acceleration; // 加速度
	Vec3 accumulated_normal;

public:
	Vec3 velocity; // 速度
	Particule(Vec3 pos) : pos(pos), velocity(Vec3(0, 0, 0)), acceleration(Vec3(0, 0, 0)), mass(2), movable(true), accumulated_normal(Vec3(0, 0, 0)) 
	{
		old_pos = pos;
	}
	Particule() {}

	void addForce(Vec3 f) {
		acceleration += f / mass;
	}

	//利用一个奇妙的物理小公式计算新的位置
	void timeStep() {
		if (movable)
		{
			if (isVerlet)
			{
				velocity += acceleration*TIME_STEPSIZE;
				Vec3 temp = pos;
				pos = pos + (pos - old_pos)*(1.0 - DAMPING) + acceleration*TIME_STEPSIZE2;
				old_pos = temp;
			}
			else
			{
				acceleration = acceleration*(1 - DAMPING);
				pos += velocity*TIME_STEPSIZE + 0.5*acceleration*TIME_STEPSIZE2;
				velocity += acceleration*TIME_STEPSIZE;
			}
		}
		acceleration = Vec3(0, 0, 0);
	}

	Vec3& getPos() {
		return pos;
	}

	void resetAcceleration() {
		acceleration = Vec3(0, 0, 0);
	}

	void offsetPos(const Vec3 v) {
		if (movable) pos += v;
	}

	void makeUnmovable() {
		movable = false;
	}

	void makeMovable() {
		movable = true;
	}

	void addToNormal(Vec3 normal) {
		accumulated_normal += normal.normalize();
	}

	Vec3& getNormal() {
		return accumulated_normal;
	}

	void resetNormal() {
		accumulated_normal = Vec3(0, 0, 0);
	}

};

// 弹簧
class Lien {
private:
	float rest_distance; //距离约束

public:
	Particule *p1, *p2; // 两个点的指针

	Lien(Particule *p1, Particule *p2) : p1(p1), p2(p2) {
		rest_distance = (p1->getPos() - p2->getPos()).length();
	}

	/* 利用阻尼器和弹簧，计算力	*/
	void lienPossible() {
		Vec3 p1_to_p2 = p2->getPos() - p1->getPos();
		double current_distance = p1_to_p2.length();
		Vec3 Force = (current_distance - rest_distance)*(p1_to_p2.normalize()) * 2;
		Force += (p2->velocity - p1->velocity)*0.3;
		p1->addForce(Force * 1);
		p2->addForce(Force*-1);
		//Vec3 correctionVector = p1_to_p2*(1 - rest_distance / current_distance); // vecteur de compensation : deplace p1 d'une distance rest_distance de p2
		//Vec3 correctionVectorHalf = correctionVector*0.5; // on prend la moitie de la longueur precedente pour bouger P1 et P2
		//p1->offsetPos(correctionVectorHalf); // correctionVectorHalf pointe de p1 a P2 pour que la longueur puisse bouger P2 de moitie pour satisfaire la creation des liens.
		//p2->offsetPos(correctionVectorHalf*-1); // on deplace p2 de -direction si on va de P2 a p1 au lieu de P1 a P2	
	}

};

// ========== 布料的定义==========
class Tissu {
private:
	std::vector<Particule> particules;
	std::vector<Lien> liens;


	int nb_particules_large; // 横向粒子数量
	int nb_particules_hauteur; // 纵向粒子数量

	Particule* getParticule(int x, int y) {
		return &particules[y*nb_particules_large + x];
	}

	void creerLien(Particule *p1, Particule *p2) {
		liens.push_back(Lien(p1, p2));
	}



	Vec3 calcTriangleNormal(Particule *p1, Particule *p2, Particule *p3) {
		Vec3 pos1 = p1->getPos();
		Vec3 pos2 = p2->getPos();
		Vec3 pos3 = p3->getPos();

		Vec3 v1 = pos2 - pos1;
		Vec3 v2 = pos3 - pos1;

		return v1^v2;
	}

	/* 对一个三角面加力，或许不应该写在布料里？*/
	void addWindForcesForTriangle(Particule *p1, Particule *p2, Particule *p3, const Vec3 direction) {
		Vec3 normal = calcTriangleNormal(p1, p2, p3);
		Vec3 d = normal.normalize();
		Vec3 force = normal*(d.dot(direction));
		p1->addForce(force);
		p2->addForce(force);
		p3->addForce(force);
	}

	/* 画出一个三角形，用Vec3强转颜色，如果用float会出错*/
	void drawTriangle(Particule *p1, Particule *p2, Particule *p3, const Vec3 color) {
		glColor3dv((double*)&color);

		glNormal3dv((double *) &(p1->getNormal().normalize()));
		glVertex3dv((double *) &(p1->getPos()));

		glNormal3dv((double *) &(p2->getNormal().normalize()));
		glVertex3dv((double *) &(p2->getPos()));

		glNormal3dv((double *) &(p3->getNormal().normalize()));
		glVertex3dv((double *) &(p3->getPos()));
	}

public:




	/*布料构造函数*/
	Tissu(float large, float hauteur, int nb_particules_large, int nb_particules_hauteur) : nb_particules_large(nb_particules_large), nb_particules_hauteur(nb_particules_hauteur) {
		particules.resize(nb_particules_large*nb_particules_hauteur);
		for (int x = 0; x < nb_particules_large; x++) {
			for (int y = 0; y < nb_particules_hauteur; y++) {
				Vec3 pos = Vec3(large * (x / (float)nb_particules_large), hauteur * (y / (float)nb_particules_hauteur), 0);
				particules[y*nb_particules_large + x] = Particule(pos);
			}
		}

		// 相邻的粒子连起来
		for (int x = 0; x < nb_particules_large; x++) {
			for (int y = 0; y < nb_particules_hauteur; y++) {
				if (x < nb_particules_large - 1) creerLien(getParticule(x, y), getParticule(x + 1, y));
				if (y < nb_particules_hauteur - 1) creerLien(getParticule(x, y), getParticule(x, y + 1));
				if (x < nb_particules_large - 1 && y < nb_particules_hauteur - 1) creerLien(getParticule(x, y), getParticule(x + 1, y + 1));
				if (x < nb_particules_large - 1 && y < nb_particules_hauteur - 1) creerLien(getParticule(x + 1, y), getParticule(x, y + 1));
			}
		}


		// 距离为2的连起来
		for (int x = 0; x < nb_particules_large; x++) {
			for (int y = 0; y < nb_particules_hauteur; y++) {
				if (x < nb_particules_large - 2) creerLien(getParticule(x, y), getParticule(x + 2, y));
				if (y < nb_particules_hauteur - 2) creerLien(getParticule(x, y), getParticule(x, y + 2));
				if (x < nb_particules_large - 2 && y < nb_particules_hauteur - 2) creerLien(getParticule(x, y), getParticule(x + 2, y + 2));
				if (x < nb_particules_large - 2 && y < nb_particules_hauteur - 2) creerLien(getParticule(x + 2, y), getParticule(x, y + 2));
			}
		}


		// 固定一条边
		for (int i = nb_particules_large / 2.5; i < nb_particules_large; i++) {
			getParticule(0 + i, 0)->offsetPos(Vec3(0.5, 0.0, 0.0)); // 更自然一点？
			getParticule(0 + i, 0)->makeUnmovable();

			//getParticule(0+i ,0)->offsetPos(Vec3(-0.5,0.0,0.0)); 
			//getParticule(nb_particules_large-1-i ,0)->makeUnmovable();
		}




	}

	/*
	(x,y)   *--* (x+1,y)
	| /|
	|/ |
	(x,y+1) *--* (x+1,y+1)

	*/


	void drawShaded() {
		// 重新计算法线
		std::vector<Particule>::iterator particule;
		for (particule = particules.begin(); particule != particules.end(); particule++) {
			(*particule).resetNormal();
		}
		for (int x = 0; x < nb_particules_large - 1; x++) {
			for (int y = 0; y < nb_particules_hauteur - 1; y++) {
				Vec3 normal = calcTriangleNormal(getParticule(x + 1, y), getParticule(x, y), getParticule(x, y + 1));
				getParticule(x + 1, y)->addToNormal(normal);
				getParticule(x, y)->addToNormal(normal);
				getParticule(x, y + 1)->addToNormal(normal);

				normal = calcTriangleNormal(getParticule(x + 1, y + 1), getParticule(x + 1, y), getParticule(x, y + 1));
				getParticule(x + 1, y + 1)->addToNormal(normal);
				getParticule(x + 1, y)->addToNormal(normal);
				getParticule(x, y + 1)->addToNormal(normal);
			}
		}

		// 逐三角形绘制
		glBegin(GL_TRIANGLES);
		for (int x = 0; x < nb_particules_large - 1; x++) {
			for (int y = 0; y < nb_particules_hauteur - 1; y++) {

				Vec3 color = Vec3(0.69, 0.13, 0.13);

				drawTriangle(getParticule(x + 1, y), getParticule(x, y), getParticule(x, y + 1), color);
				drawTriangle(getParticule(x + 1, y + 1), getParticule(x + 1, y), getParticule(x, y + 1), color);
			}
		}
		glEnd();
	}

	/* 迭代计算*/
	void timeStep() {
		std::vector<Lien>::iterator lien;
		for (int i = 0; i < LIENS_ITERATIONS; i++) {//约束迭代次数 已经废弃
			for (lien = liens.begin(); lien != liens.end(); lien++) {
				(*lien).lienPossible();
			}
		}

		std::vector<Particule>::iterator particule;
		for (particule = particules.begin(); particule != particules.end(); particule++) {
			(*particule).timeStep(); // 逐粒子重新计算
		}
	}


	void addForce(const Vec3 direction) {
		std::vector<Particule>::iterator particule;
		for (particule = particules.begin(); particule != particules.end(); particule++) {
			(*particule).addForce(direction);
		}

	}

	void windForce(const Vec3 direction) {
		for (int x = 0; x < nb_particules_large - 1; x++) {
			for (int y = 0; y < nb_particules_hauteur - 1; y++) {
				addWindForcesForTriangle(getParticule(x + 1, y), getParticule(x, y), getParticule(x, y + 1), direction);
				addWindForcesForTriangle(getParticule(x + 1, y + 1), getParticule(x + 1, y), getParticule(x, y + 1), direction);
			}
		}
	}

	void ballCollision(const Vec3 center, const float radius) {
		std::vector<Particule>::iterator particule;
		for (particule = particules.begin(); particule != particules.end(); particule++) {
			Vec3 v = (*particule).getPos() - center;
			float l = v.length();
			if (l < radius) {
				(*particule).offsetPos(v.normalize()*(radius - l));
			}
		}
	}


	void cubeCollision(double cube_size, const Vec3 cube_pos) {
		std::vector<Particule>::iterator particule;
		for (particule = particules.begin(); particule != particules.end(); particule++) {
			Vec3 v = (*particule).getPos() - cube_pos;
			double l = max(max(abs(v.x), abs(v.y)), abs(v.z));
			//double l = v.distanceCube();
			//std::cout << "distance cube : " << l << std::endl;
			if (l < (cube_size)) {
				(*particule).offsetPos(v.normalize()*1.05*(cube_size - l));
				particule->velocity = Vec3(0, 0, 0);
			}
		}
	}

	void doFrame() {

	}
};



Tissu drap(15, 10, 60, 40); // 布料
Vec3 ball_pos(7, -5, 0); // 球
Vec3 cube_pos(10, -5, 0); // 正方体
float ball_radius = 2;
float ball_time = 0;
float cube_size = 3.5;
int ball = 0; // 是否开启『球』
double camera_theta = 3 * PI / 2.0;
double camera_phi = 0.0;
double camera_r = 30.0;
Vec3 Gravity = Vec3(0, -0.01, 0);
Vec3 WindForce = Vec3(0.004, 0, -0.04);
float density = 0.03;
// ========== INIT ==========

void init(void) {
	glShadeModel(GL_SMOOTH);
	glClearColor(0.2f, 0.2f, 0.4f, 0.5f);
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_COLOR_MATERIAL);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);


	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	GLfloat lightPos[4] = { -1.0,1.0,0.5,0.0 };
	glLightfv(GL_LIGHT0, GL_POSITION, (GLfloat *)&lightPos);

	glEnable(GL_LIGHT1);

	GLfloat lightAmbient1[4] = { 0.0,0.0,0.0,0.0 };
	GLfloat lightPos1[4] = { 1.0,0.0,-0.2,0.0 };
	GLfloat lightDiffuse1[4] = { 0.5,0.5,0.3,0.0 };

	glLightfv(GL_LIGHT1, GL_POSITION, (GLfloat *)&lightPos1);
	glLightfv(GL_LIGHT1, GL_AMBIENT, (GLfloat *)&lightAmbient1);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, (GLfloat *)&lightDiffuse1);

	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);

	GLfloat fogColor[4] = { 0.5, 0.5, 0.5, 1.0 };
	glEnable(GL_FOG);
	glFogi(GL_FOG_MODE, GL_EXP2);
	glFogfv(GL_FOG_COLOR, fogColor);
	glFogf(GL_FOG_DENSITY, density);
	glHint(GL_FOG_HINT, GL_NICEST);
}




// ========== DRAW ==========
void draw(void) {

	//让球运动
	ball_time++;
	ball_pos.y = cos(ball_time / 50.0) * 7;

	//给布料一个力，并开始迭代
	drap.addForce(Gravity);
	drap.windForce(WindForce);
	drap.timeStep();

	//碰撞检测
	if (ball == 1) drap.ballCollision(ball_pos, ball_radius);
	drap.cubeCollision(cube_size, cube_pos);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
	gluLookAt(camera_r*sin(camera_theta)*cos(camera_phi), camera_r*cos(camera_theta), camera_r*sin(camera_theta)*sin(camera_phi),
		0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
	glDisable(GL_LIGHTING);
	glBegin(GL_POLYGON);
	glColor3f(1.0f, 1.0f, 1.0f);
	glVertex3f(-200.0f, -100.0f, -100.0f);
	glVertex3f(200.0f, -100.0f, -100.0f);
	glColor3f(0.4f, 0.4f, 0.8f);
	glVertex3f(200.0f, 100.0f, -100.0f);
	glVertex3f(-200.0f, 100.0f, -100.0f);
	glEnd();
	glEnable(GL_LIGHTING);
	//画布料
	drap.drawShaded();


	glPushMatrix();
	glTranslated(ball_pos.x, ball_pos.y, ball_pos.z);
	glColor3f(0.6f, 0.19f, 0.8f);
	if (ball == 1) {
		glutSolidSphere(ball_radius - 0.1, 50, 50);
	}
	glPopMatrix();


	//画立方体
	glPushMatrix();
	glTranslated(cube_pos.x, cube_pos.y, cube_pos.z);
	glColor3f(0.27f, 0.5f, 0.7f);
	glutSolidCube(1.7*cube_size);
	glPopMatrix();



	glutSwapBuffers();
	glutPostRedisplay();
}

// ==========  ==========
void reshape(int w, int h) {
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	if (h == 0)
		gluPerspective(80, (float)w, 1.0, 5000.0);
	else
		gluPerspective(80, (float)w / (float)h, 1.0, 5000.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

// ========== KEYBOARD ==========
void keyboard(unsigned char key, int x, int y) {
	string s;
	switch (key) {
	case 'z':
		exit(0);
		break;
	case 'f':
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // Texture
		glutPostRedisplay();
		break;
	case 'l':
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // WireFrame
		glutPostRedisplay();
		break;
	case 'p':
		glPolygonMode(GL_FRONT_AND_BACK, GL_POINT); // Points
		glutPostRedisplay();
		break;
	case 'b': // 球
		if (ball == 0) {
			ball = 1;
		}
		else {
			ball = 0;
		}
		glutPostRedisplay();
		break;
	case 'w':
		if (camera_r - 1.0 / 18.0 > 0)
			camera_r -= 1.0;
		break;
	case 's':
		camera_r += 1.0;
		break;
	case 'q':
		camera_theta += PI / 90;
		break;
	case 'e':
		camera_theta -= PI / 90;
		break;
	case 'a':
		camera_phi += PI / 90;
		break;
	case 'd':
		camera_phi -= PI / 90;
		break;
	case 'c':
		cin >> s;
		if (s == "w" || s == "W")
		{
			cin >> WindForce.x >> WindForce.y >> WindForce.z;
			cout << "Update WindForce Successfully!" << endl;
		}
		else if (s == "g" || s == "G")
		{
			cin >> Gravity.y;
			cout << "Update Gravity Successfully!" << endl;
		}
		break;
	case 'o':
		cout << "Communication University of China" << endl;
		cout << "BY Wxzuir 2017-12-21" << endl;
		break;
	default:
		break;
	}
}

// ========== SPECIAL KEYS ==========
//全屏模式
void arrow_keys(int a_keys, int x, int y) {
	switch (a_keys) {
	case GLUT_KEY_UP:
		glutFullScreen();
		break;
	case GLUT_KEY_DOWN:
		glutReshapeWindow(1000, 700);
		break;
	default:
		break;
	}
}


// ========== MAIN ==========
int main(int argc, char** argv) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(1000, 700);

	glutCreateWindow("Drapeau");
	init();
	glutDisplayFunc(draw);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyboard);
	glutSpecialFunc(arrow_keys);

	glutMainLoop();
}
