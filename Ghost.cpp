#include "Ghost.hpp"

GLuint Ghost::redghostlist;

Ghost::Ghost(float _x, float _z){
	x = _x;
	z = _z;
	dir = 5;
	moved = 1.f;
}

void Ghost::init(GLuint _redghostlist){
	redghostlist = _redghostlist;
}

void Ghost::update(float dt, Map& map){
	bool newDir = false;

	float toMove = dt*MOVESPEED;
	if(moved + toMove > 1.0f){
		toMove = moved + toMove - 1.0f;
		moved = 0.f;
		newDir = true;
	}
	moved += toMove;

	switch(dir){
		case 0: z -= toMove; break;
		case 1: x += toMove; break;
		case 2: z += toMove; break;
		case 3: x -= toMove; break;
	}

	int olddir = dir;
	if(newDir){
		if(map.canMove(xmask(x,dir),zmask(z,dir))){
			for(int i = 0; i < 4; ++i) {
				dir = (dir - 1 + (rand() % 3))%4;
				if(dir != (olddir+2)%4 && map.canMove(xmask(x,dir),zmask(z,dir))){
					std::cout << dir << std::endl;
					return;
				}
			}
		}
		else{
			for(int i = 0; i < 4; ++i) {
				dir = (dir + 1 + (rand() % 3))%4;
				if(dir != olddir && map.canMove(xmask(x,dir),zmask(z,dir))){
					std::cout << dir << std::endl;
					return;
				}
			}
		}
	}
}

int Ghost::xmask(int nx, int ndir){
	if(ndir == 1)
		return nx+1;
	if(ndir == 3)
		return nx-1;
	return nx;
}

int Ghost::zmask(int nz, int ndir){
	if(ndir == 0)
		return nz-1;
	if(dir == 2)
		return nz+1;
	return nz;
}

void Ghost::draw(float dirdeg){
	glPushMatrix();
	glTranslatef(x,0,z);
	glRotatef(-dirdeg,0,1,0);
	glCallList(redghostlist);
	glPopMatrix();
}
