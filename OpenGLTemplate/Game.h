#pragma once

#include "Common.h"
#include "GameWindow.h"

// Classes used in game.  For a new class, declare it here and provide a pointer to an object of this class below.  Then, in Game.cpp, 
// include the header.  In the Game constructor, set the pointer to NULL and in Game::Initialise, create a new object.  Don't forget to 
// delete the object in the destructor.   
class CCamera;
class CSkybox;
class CShader;
class CShaderProgram;
class CPlane;
class CFreeTypeFont;
class CHighResolutionTimer;
class CSphere;
class COpenAssetImportMesh;
class CAudio;
class CCatmullRom;

class Game 
{
private:
	// The position of the camera's centre of projection
	glm::vec3 camera_position;		
	// The camera's viewpoint (point where the camera is looking)
	glm::vec3 camera_view;
	// The camera's up vector
	glm::vec3 camera_upVector;			
	//camera spped
	float m_cameraSpeed;

	//float m_playerCurrentDistance;
	//float lookAtAngle;
	//glm::vec3 curPlayerDir;

	int debug;

	glm::vec3 zero_vector;

	//player current position
	glm::vec3 player_position;
	glm::mat4 player_orientation;
	glm::vec3 player_lookAt;
	glm::mat4 playerTf;
	float playerTOffset = 0;

	float ringCout;
	glm::mat4 obstacleTf[20];

	int scores;
	// distance along the control path we’ve travelled
	float m_currentDistance;

	// Some other member variables
	double m_dt;
	int m_framesPerSecond;
	bool m_appActive;

	static const int FPS = 60;
	int m_frameCount;
	double m_elapsedTime;

	GameWindow m_gameWindow;
	HINSTANCE m_hInstance;

	// Pointers to game objects.  
	//They will get allocated in Game::Initialise()
	CSkybox *m_pSkybox;
	CCamera *m_pCamera;
	vector <CShaderProgram *> *m_pShaderPrograms;
	CPlane *m_pPlanarTerrain;
	CFreeTypeFont *m_pFtFont;
	COpenAssetImportMesh *m_RingMesh;
	COpenAssetImportMesh *m_ShipMesh;
	CSphere *m_pSphere;
	CHighResolutionTimer *m_pHighResolutionTimer;
	CAudio *m_pAudio;
	CCatmullRom *m_pCatmullRom;

private:
	// Three main methods used in the game.  Initialise runs once, while Update and Render run repeatedly in the game loop.
	void Initialise();
	void Update();
	void Render();
	void DisplayFrameRate();
	void Game::AddRings();
	void UI();
	void GameLoop();

public:
	Game();
	~Game();
	static Game& GetInstance();
	LRESULT ProcessEvents(HWND window,UINT message, WPARAM w_param, LPARAM l_param);
	void SetHinstance(HINSTANCE hinstance);
	WPARAM Execute();
};
