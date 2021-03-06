#include "Game.hpp"

Game::Game(){
	running = false;
	lampOn = true;
	lampTime = 0.f;
	paused = false;
}

/*
 * Main game loop.
 */
void Game::loop(){
	running = true;

	// Fade in
	fade(0,2.f);

	sf::Mouse::SetPosition(sf::Vector2i(SCREEN_WIDTH/2,SCREEN_HEIGHT/2),app);
	while(running){
		float time = clock.GetElapsedTime()/1000.f;
		clock.Reset();
		while(app.PollEvent(event)){
			if(event.Type == sf::Event::Closed){
				running = false;
			}
			else if(event.Type == sf::Event::KeyPressed){
				if(event.Key.Code == sf::Keyboard::P){
					paused = !paused;
				}
			}
			else if(event.Type == sf::Event::GainedFocus){
				hasFocus = true;
			}
			else if(event.Type == sf::Event::LostFocus){
				hasFocus = false;
			}
		}

		if(!paused){
			// Update player
			pl.update(time, map, app, hasFocus, sndmgr);
			if(pl.collideDots(dots,sndmgr) == 1){
				for(git = ghosts.begin(); git < ghosts.end(); ++git){
					git->setScared();	
				}
			}
			// Update ghosts
			for(git = ghosts.begin(); git < ghosts.end(); ++git) {
				if(git->alive){
					git->update(time,map);	
				}
				else{
					particles.push_back(Particle(git->x,0,git->z,particleKillGhost));
					git->respawn();
				}
			}

			// Update particles
			for(pit = particles.begin(); pit < particles.end(); ++pit) {
				if(pit->alive){
					pit->update(time);	
				}
				else{
					particles.erase(pit);
				}
			}
			pl.collideGhosts(ghosts,sndmgr);

			// Update lamp blinking
			if(lampTime < 0){
				if(lampOn)
					lampTime = (float)(rand() % 10 + 5)/100.f;
				else
					lampTime = (float)(rand() % 50 + 10)/100.f;
				lampOn = !lampOn;
			}
			lampTime -= time;
		}

		// Clear screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		// Draw objects
		draw();
		// Draw 2D effects
		draw2D();
		// redraw screen
		app.Display();
	}

	// fade out
	fade(1,1.f);
}

/*
 * Translates screen to player position
 * and draws all 3D objects in world, including billboard sprites.
 */
void Game::draw(){
	glLoadIdentity();
	// Rotate view
	glRotatef(pl.ydirdeg,1,0,0);
	glRotatef(pl.xdirdeg,0,1,0);

	// Translate according to player coords,
	// draw shadow in between translating y axis
	glTranslatef(0,-pl.y,0);
	glCallList(shadow);
	glTranslatef(-pl.x,0,-pl.z);

	// Draw walls/tiles
	drawWalls();

	// Draw dots
	for(int i = 0; i < dots.size(); ++i) {
		dots.at(i).draw(pl.xdirdeg);
	}
	// Draw ghosts
	for(git = ghosts.begin(); git < ghosts.end(); ++git){
		git->draw(pl.xdirdeg);
	}

	// Draw particles
	for(pit = particles.begin(); pit < particles.end(); ++pit) {
		pit->draw(pl.xdirdeg);
	}
}

/*
 * Switches to orthogonal matrix
 * and draws 2D screen effects (white noise, red etc.)
 */
void Game::draw2D(){
	pushOrtho();
		pl.drawEffects();
	popOrtho();
}

/*
 * Fades screen to or from black.
 * Objets are not updated while fading.
 *
 * dir Determines fade direction. 0 for form black, 1 for to black.
 * fadetime Determines number of seconds to fade.
 */
void Game::fade(int dir, float fadetime){
	float starttime = fadetime;
	float time = starttime;

	while(time > 0){
		float dt = clock.GetElapsedTime()/1000.f;
		clock.Reset();
		time -= dt;	

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		draw();

		float alpha = time/starttime;
		if(dir == 1){
			alpha = 1-alpha;
		}

		glColor4f(0.f,0.f,0.f,alpha);
		pushOrtho();
			glBegin(GL_QUADS);
				glVertex2f(0,0);
				glVertex2f(SCREEN_WIDTH,0);
				glVertex2f(SCREEN_WIDTH,SCREEN_HEIGHT);
				glVertex2f(0,SCREEN_HEIGHT);
			glEnd();
		popOrtho();
		glColor4f(1.f,1.f,1.f,1.f);
		app.Display();
	}
}

/*
 * Loops through map and draws tiles using
 * display lists.
 */
void Game::drawWalls(){
	glPushMatrix();
	// Draw walls, floors
	for(int iy = 0; iy < map.h; ++iy) {
		for(int ix = 0; ix < map.w; ++ix) {
			char tile = map.data[iy*map.w+ix];
			switch(tile){
				case 0:
					glCallList(floor);
					glCallList(ceiling);
					break;
				case 1:
					glCallList(walls);
					break;
				case 2:
					if(lampOn){
						glCallList(lampfloor);
						glCallList(lampceiling);
					}
					else{
						glCallList(floor);
						glCallList(lampceilingoff);
					}
					break;
				case 3:
					glCallList(portal);
					break;
			}
			glTranslatef(1,0,0);
		}
		glTranslatef(-map.w,0,1);
	}
	glPopMatrix();
}

/*
 * Switches to orthogonal matrix for 2D drawing
 */
void Game::pushOrtho(){
	glEnable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_DEPTH_TEST);

	glPushMatrix();
	glLoadIdentity();
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(0,SCREEN_WIDTH,0,SCREEN_HEIGHT,-1.f,1.f);
}

/*
 * Pops original perspective matrix.
 * To be used after pushOrtho()
 */
void Game::popOrtho(){
		glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	glDisable(GL_BLEND);
	glEnable(GL_ALPHA_TEST);
	glEnable(GL_DEPTH_TEST);
}

int Game::execute(){
	if(init() == false){
		return -1;
	}
	loop();
	return 0;
}

/*
 * Creates SFML window and sets up OpenGL settings.
 */
bool Game::init(){
	app.Create(sf::VideoMode(SCREEN_WIDTH,SCREEN_HEIGHT,SCREEN_BPP),"FPS",sf::Style::Close);
	if(app.IsOpened() == false){
		return false;
	}
	app.ShowMouseCursor(false);

	compileDisplayLists();

	Pickup::init(smalldot);
	Ghost::init(redghost);

	if(loadResources() == false){
		return false;
	}

	srand((int)(clock.GetElapsedTime()*1000.f));

	// Init projection matrix
	glViewport(0,0,SCREEN_WIDTH,SCREEN_HEIGHT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(80.f,(GLfloat)SCREEN_WIDTH/(GLfloat)SCREEN_HEIGHT,0.01f,100.f);
	// Init model view matrix
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	// Background color
	glClearColor(0.f,0.f,0.f,0.f);
	// Depth buffer
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_TEXTURE_2D);
	// Blend function
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_BLEND);
	// Only allow pixels with alpha > 0.9 to be drawn
	glAlphaFunc(GL_GREATER,0.9f);
	glEnable(GL_ALPHA_TEST);
	// Fog stuff
	GLfloat fogColor[] = {0.f,0.f,0.f,1.f};
	glClearColor(0.0f,0.0f,0.0f,1.0f);
	glFogi(GL_FOG_MODE, GL_EXP2);
	glFogfv(GL_FOG_COLOR, fogColor);
	glFogf(GL_FOG_DENSITY, 0.65f);
	glHint(GL_FOG_HINT, GL_FASTEST);
	glFogf(GL_FOG_START, 0.5f);
	glFogf(GL_FOG_END, 6.0f);
	glEnable(GL_FOG);

	return true;
}

/*
 * Loads resoures from files.
 * Returns false if something fails to load.
 */
bool Game::loadResources(){
	if(tiles.LoadFromFile("res/tiles.png") == false){
		return false;
	}
	Game::tiles.SetSmooth(false);
	Game::tiles.Bind();

	if(map.readFromImage("res/levels/1.png",dots,ghosts) == false){
		return false;
	}
	pl.x = map.startx;
	pl.z = map.startz;

	if(sndmgr.loadSounds() == false){
		return false;
	}

	return true;
}

int main(int argc, char** argv){
	Game game;
	return game.execute();
}
