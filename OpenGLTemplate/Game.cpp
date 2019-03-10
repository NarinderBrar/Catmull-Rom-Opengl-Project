/* 
OpenGL Template for INM376 / IN3005
City University London, School of Mathematics, Computer Science and Engineering
Source code drawn from a number of sources and examples, including contributions from
 - Ben Humphrey (gametutorials.com), Michal Bubner (mbsoftworks.sk), Christophe Riccio (glm.g-truc.net)
 - Christy Quinn, Sam Kellett and others

 For educational use by Department of Computer Science, City University London UK.

 This template contains a skybox, simple terrain, camera, lighting, shaders, texturing

 Potential ways to modify the code:  Add new geometry types, shaders, change the terrain, load new meshes, change the lighting, 
 different camera controls, different shaders, etc.
 
 Template version 5.0a 29/01/2017
 Dr Greg Slabaugh (gregory.slabaugh.1@city.ac.uk) 
*/

#include "game.h"

// Setup includes
#include "HighResolutionTimer.h"
#include "GameWindow.h"
#include <iostream>

// Game includes
#include "Camera.h"
#include "Skybox.h"
#include "Plane.h"
#include "Shaders.h"
#include "FreeTypeFont.h"
#include "Sphere.h"
#include "MatrixStack.h"
#include "OpenAssetImportMesh.h"
#include "Audio.h"
#include "CCatmullRom.h"

// Constructor
Game::Game()
{
	m_pSkybox = NULL;
	m_pCamera = NULL;
	m_pShaderPrograms = NULL;
	m_pPlanarTerrain = NULL;
	m_pFtFont = NULL;

	m_RingMesh = NULL;

	m_ShipMesh = NULL;
	m_pSphere = NULL;
	m_pHighResolutionTimer = NULL;
	m_pAudio = NULL;

	m_dt = 0.0;
	m_framesPerSecond = 0;
	m_frameCount = 0;
	m_elapsedTime = 0.0f;

	m_pCatmullRom = NULL;

	m_currentDistance = 20.0f;
	//m_playerCurrentDistance = 0.01f;
	m_cameraSpeed = 0.05f;

	ringCout = 20;
	scores = 0;
}

// Destructor
Game::~Game() 
{ 
	//game objects
	delete m_pCamera;
	delete m_pSkybox;
	delete m_pPlanarTerrain;
	delete m_pFtFont;
	delete m_RingMesh;
	delete m_ShipMesh;
	delete m_pSphere;
	delete m_pAudio;

	delete m_pCatmullRom;

	if (m_pShaderPrograms != NULL) {
		for (unsigned int i = 0; i < m_pShaderPrograms->size(); i++)
			delete (*m_pShaderPrograms)[i];
	}
	delete m_pShaderPrograms;

	//setup objects
	delete m_pHighResolutionTimer;
}

// Initialisation:  This method only runs once at startup
void Game::Initialise() 
{
	// Set the clear colour and depth
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f);

	zero_vector = glm::vec3(0.0f, 0.0f, 0.0f);

	camera_view = glm::vec3(0, 0.0f, 0.0f);
	camera_upVector = glm::vec3(0.0f, 1.0f, 0.0f);

	//curPlayerDir = glm::vec3(1, 0, 0);;

	playerTOffset = 0;
	// Create objects
	m_pCamera = new CCamera;
	m_pSkybox = new CSkybox;
	m_pShaderPrograms = new vector <CShaderProgram *>;
	m_pPlanarTerrain = new CPlane;
	m_pFtFont = new CFreeTypeFont;
	m_RingMesh = new COpenAssetImportMesh;
	m_ShipMesh = new COpenAssetImportMesh;
	m_pSphere = new CSphere;
	m_pAudio = new CAudio;

	RECT dimensions = m_gameWindow.GetDimensions();

	int width = dimensions.right - dimensions.left;
	int height = dimensions.bottom - dimensions.top;

	// Set the orthographic and perspective projection matrices based on the image size
	m_pCamera->SetOrthographicProjectionMatrix(width, height); 
	m_pCamera->SetPerspectiveProjectionMatrix(45.0f, (float) width / (float) height, 0.5f, 5000.0f);

	// Load shaders
	vector<CShader> shShaders;
	vector<string> sShaderFileNames;
	sShaderFileNames.push_back("mainShader.vert");
	sShaderFileNames.push_back("mainShader.frag");
	sShaderFileNames.push_back("textShader.vert");
	sShaderFileNames.push_back("textShader.frag");

	for (int i = 0; i < (int) sShaderFileNames.size(); i++) 
	{
		string sExt = sShaderFileNames[i].substr((int) sShaderFileNames[i].size()-4, 4);
		int iShaderType;
		if (sExt == "vert") iShaderType = GL_VERTEX_SHADER;
		else if (sExt == "frag") iShaderType = GL_FRAGMENT_SHADER;
		else if (sExt == "geom") iShaderType = GL_GEOMETRY_SHADER;
		else if (sExt == "tcnl") iShaderType = GL_TESS_CONTROL_SHADER;
		else iShaderType = GL_TESS_EVALUATION_SHADER;
		CShader shader;
		shader.LoadShader("resources\\shaders\\"+sShaderFileNames[i], iShaderType);
		shShaders.push_back(shader);
	}

	// Create the main shader program
	CShaderProgram *pMainProgram = new CShaderProgram;
	pMainProgram->CreateProgram();
	pMainProgram->AddShaderToProgram(&shShaders[0]);
	pMainProgram->AddShaderToProgram(&shShaders[1]);
	pMainProgram->LinkProgram();
	m_pShaderPrograms->push_back(pMainProgram);

	// Create a shader program for fonts
	CShaderProgram *pFontProgram = new CShaderProgram;
	pFontProgram->CreateProgram();
	pFontProgram->AddShaderToProgram(&shShaders[2]);
	pFontProgram->AddShaderToProgram(&shShaders[3]);
	pFontProgram->LinkProgram();
	m_pShaderPrograms->push_back(pFontProgram);

	// You can follow this pattern to load additional shaders
	// Create the skybox
	// Skybox downloaded from http://www.akimbo.in/forum/viewtopic.php?f=10&t=9
	m_pSkybox->Create(2500.0f);
	
	// Create the planar terrain
	m_pPlanarTerrain->Create("resources\\textures\\", "tiles.jpg", 2000.0f, 2000.0f, 50.0f); // Texture downloaded from http://www.psionicgames.com/?page_id=26 on 24 Jan 2013

	m_pFtFont->LoadSystemFont("arial.ttf", 32);
	m_pFtFont->SetShaderProgram(pFontProgram);

	// Load some meshes in OBJ format
	m_RingMesh->Load("resources\\models\\Glow\\glow.obj");  // Downloaded from http://www.psionicgames.com/?page_id=24 on 24 Jan 2013
	m_ShipMesh->Load("resources\\models\\Spaceship\\spaceship.obj");  // Downloaded from http://opengameart.org/content/horse-lowpoly on 24 Jan 2013

	// Create a sphere
	m_pSphere->Create("resources\\textures\\", "dirtpile01.jpg", 25, 25);  // Texture downloaded from http://www.psionicgames.com/?page_id=26 on 24 Jan 2013
	glEnable(GL_CULL_FACE);

	// Initialise audio and play background music
	m_pAudio->Initialise();
	m_pAudio->LoadEventSound("Resources\\Audio\\Boing.wav");					// Royalty free sound from freesound.org
	m_pAudio->LoadMusicStream("Resources\\Audio\\DST-Garote.mp3");	// Royalty free music from http://www.nosoapradio.us/
	m_pAudio->PlayMusicStream();

	m_pCatmullRom = new CCatmullRom;
	m_pCatmullRom->CreateCentreline();

	m_pCatmullRom->CreateOffsetCurves();

	m_pCatmullRom->CreateTrack();

	AddRings();
}

void Game::AddRings()
{
	int m_vertexCount = (int)m_pCatmullRom->m_centrelinePoints.size();

	int j = 0;

	int max = 0;
	int min = 0;
	int range = 0;
	int num = 0;
	
	for (size_t i = 0; i < ringCout && j < m_vertexCount-10; i++)
	{
		obstacleTf[i] = glm::mat4();
		glm::vec3 point = m_pCatmullRom->m_centrelinePoints[j];

		glm::vec3 T = glm::normalize(m_pCatmullRom->m_centrelinePoints[j+1] - point);
		glm::vec3 N = glm::normalize(glm::cross(T, glm::vec3(0, 1, 0)));
		glm::vec3 B = glm::normalize(glm::cross(N, T));

		glm::vec3 OffsetN;

		if(rand()%10 > 5)
			OffsetN = glm::vec3(N.x * 3, N.y * 3, N.z * 3);
		else
			OffsetN = glm::vec3(N.x * -3, N.y * -3, N.z * -3);

		obstacleTf[i] = glm::translate(point + glm::vec3(0, 1.5f, 0) + OffsetN);
		glm::mat4 obstacle_orientation = glm::mat4(glm::mat3(T, B, N));

		obstacleTf[i] *= obstacle_orientation;
		
		if (i < 3)
		{
			max = 100;
			min = 80;
			range = max - min + 1;
			num = rand() % range + min;
		}
		else if (i >= 3 && i < 8)
		{
			max = 50;
			min = 20;
			range = max - min + 1;
			num = rand() % range + min;
		}
		else
		{
			max = 20;
			min = 10;
			range = max - min + 1;
			num = rand() % range + min;
		}

		j += num;
	}
}

// Render method runs repeatedly in a loop
void Game::Render() 
{
	// Clear the buffers and enable depth testing (z-buffering)
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	// Set up a matrix stack
	glutil::MatrixStack modelViewMatrixStack;
	modelViewMatrixStack.SetIdentity();

	// Use the main shader program 
	CShaderProgram *pMainProgram = (*m_pShaderPrograms)[0];
	pMainProgram->UseProgram();
	pMainProgram->SetUniform("bUseTexture", true);
	pMainProgram->SetUniform("sampler0", 0);
	
	// Note: cubemap and non-cubemap textures should not be mixed in the same texture unit.  Setting unit 10 to be a cubemap texture.
	int cubeMapTextureUnit = 10; 
	pMainProgram->SetUniform("CubeMapTex", cubeMapTextureUnit);
	
	// Set the projection matrix
	pMainProgram->SetUniform("matrices.projMatrix", m_pCamera->GetPerspectiveProjectionMatrix());

	// Call LookAt to create the view matrix and put this on the modelViewMatrix stack. 
	// Store the view matrix and the normal matrix associated with the view matrix for later (they're useful for lighting -- since lighting is done in eye coordinates)
	modelViewMatrixStack.LookAt(m_pCamera->GetPosition(), m_pCamera->GetView(), m_pCamera->GetUpVector());
	glm::mat4 viewMatrix = modelViewMatrixStack.Top();
	glm::mat3 viewNormalMatrix = m_pCamera->ComputeNormalMatrix(viewMatrix);

	// Set light and materials in main shader program
	glm::vec4 lightPosition1 = glm::vec4(-100, 100, -100, 1); // Position of light source *in world coordinates*
	pMainProgram->SetUniform("light1.position", viewMatrix*lightPosition1); // Position of light source *in eye coordinates*
	pMainProgram->SetUniform("light1.La", glm::vec3(1.0f));		// Ambient colour of light
	pMainProgram->SetUniform("light1.Ld", glm::vec3(1.0f));		// Diffuse colour of light
	pMainProgram->SetUniform("light1.Ls", glm::vec3(1.0f));		// Specular colour of light
	pMainProgram->SetUniform("material1.Ma", glm::vec3(1.0f));	// Ambient material reflectance
	pMainProgram->SetUniform("material1.Md", glm::vec3(0.0f));	// Diffuse material reflectance
	pMainProgram->SetUniform("material1.Ms", glm::vec3(0.0f));	// Specular material reflectance
	pMainProgram->SetUniform("material1.shininess", 15.0f);		// Shininess material property
		
	// Render the skybox and terrain with full ambient reflectance 
	modelViewMatrixStack.Push();
		pMainProgram->SetUniform("renderSkybox", true);
		// Translate the modelview matrix to the camera eye point so skybox stays centred around camera
		glm::vec3 vEye = m_pCamera->GetPosition();
		modelViewMatrixStack.Translate(vEye);
		pMainProgram->SetUniform("matrices.modelViewMatrix", modelViewMatrixStack.Top());
		pMainProgram->SetUniform("matrices.normalMatrix", m_pCamera->ComputeNormalMatrix(modelViewMatrixStack.Top()));
		m_pSkybox->Render(cubeMapTextureUnit);
		pMainProgram->SetUniform("renderSkybox", false);
	modelViewMatrixStack.Pop();

	// Render the planar terrain
	modelViewMatrixStack.Push();
	modelViewMatrixStack.Translate(glm::vec3(0.0f, -1.0f, 0.0f));
		pMainProgram->SetUniform("matrices.modelViewMatrix", modelViewMatrixStack.Top());
		pMainProgram->SetUniform("matrices.normalMatrix", m_pCamera->ComputeNormalMatrix(modelViewMatrixStack.Top()));
		m_pPlanarTerrain->Render();
	modelViewMatrixStack.Pop();


	// Turn on diffuse + specular materials
	pMainProgram->SetUniform("material1.Ma", glm::vec3(0.5f));	// Ambient material reflectance
	pMainProgram->SetUniform("material1.Md", glm::vec3(0.5f));	// Diffuse material reflectance
	pMainProgram->SetUniform("material1.Ms", glm::vec3(1.0f));	// Specular material reflectance	


	// Render the horse 
	modelViewMatrixStack.Push();
	modelViewMatrixStack.ApplyMatrix(playerTf);
		pMainProgram->SetUniform("matrices.modelViewMatrix", modelViewMatrixStack.Top());
		pMainProgram->SetUniform("matrices.normalMatrix", m_pCamera->ComputeNormalMatrix(modelViewMatrixStack.Top()));
		m_ShipMesh->Render();
	modelViewMatrixStack.Pop();

	for (size_t i = 0; i < ringCout; i++)
	{
		// Render the barrel 
		modelViewMatrixStack.Push();
		modelViewMatrixStack.ApplyMatrix(obstacleTf[i]);
		pMainProgram->SetUniform("matrices.modelViewMatrix", modelViewMatrixStack.Top());
		pMainProgram->SetUniform("matrices.normalMatrix", m_pCamera->ComputeNormalMatrix(modelViewMatrixStack.Top()));
		m_RingMesh->Render();
		modelViewMatrixStack.Pop();
	}

	// Render the sphere
	modelViewMatrixStack.Push();
		modelViewMatrixStack.Translate(glm::vec3(0.0f, 2.0f, 150.0f));
		modelViewMatrixStack.Scale(2.0f);
		pMainProgram->SetUniform("matrices.modelViewMatrix", modelViewMatrixStack.Top());
		pMainProgram->SetUniform("matrices.normalMatrix", m_pCamera->ComputeNormalMatrix(modelViewMatrixStack.Top()));
		// To turn off texture mapping and use the sphere colour only (currently white material), uncomment the next line
		//pMainProgram->SetUniform("bUseTexture", false);
		m_pSphere->Render();
	modelViewMatrixStack.Pop();
		
		modelViewMatrixStack.Push();
		modelViewMatrixStack.Translate(glm::vec3(0.0f, 0.5f, 0.0f));
		pMainProgram->SetUniform("bUseTexture", false); // turn off texturing
		pMainProgram->SetUniform("matrices.modelViewMatrix", modelViewMatrixStack.Top());
		pMainProgram->SetUniform("matrices.normalMatrix",
		m_pCamera->ComputeNormalMatrix(modelViewMatrixStack.Top()));
		m_pCatmullRom->RenderCentreline();
	modelViewMatrixStack.Pop();

		modelViewMatrixStack.Push();
		modelViewMatrixStack.Translate(glm::vec3(0.0f, 0.0f, 0.0f));
		pMainProgram->SetUniform("bUseTexture", true); // turn off texturing
		pMainProgram->SetUniform("matrices.modelViewMatrix", modelViewMatrixStack.Top());
		pMainProgram->SetUniform("matrices.normalMatrix",
		m_pCamera->ComputeNormalMatrix(modelViewMatrixStack.Top()));
		m_pCatmullRom->RenderOffsetCurves();
	modelViewMatrixStack.Pop();


/*	modelViewMatrixStack.Push();
	modelViewMatrixStack.Translate(glm::vec3(0.0f, 0.0f, 0.0f));
	pMainProgram->SetUniform("matrices.modelViewMatrix", modelViewMatrixStack.Top());
	pMainProgram->SetUniform("matrices.normalMatrix",
	m_pCamera->ComputeNormalMatrix(modelViewMatrixStack.Top()));
	m_pCatmullRom->RenderTrack();
	modelViewMatrixStack.Pop();*/


	// Render the sphere
	modelViewMatrixStack.Push();
	modelViewMatrixStack.Translate(glm::vec3(0.0f, 0.0f, 0.0f));
	pMainProgram->SetUniform("matrices.modelViewMatrix", modelViewMatrixStack.Top());
	pMainProgram->SetUniform("matrices.normalMatrix", m_pCamera->ComputeNormalMatrix(modelViewMatrixStack.Top()));
	// To turn off texture mapping and use the sphere colour only (currently white material), uncomment the next line
	//pMainProgram->SetUniform("bUseTexture", false);
	m_pCatmullRom->RenderTrack();
	modelViewMatrixStack.Pop();

	// Draw the 2D graphics after the 3D graphics
	DisplayFrameRate();

	UI();

	// Swap buffers to show the rendered image
	SwapBuffers(m_gameWindow.Hdc());		
}

float AngleBetweenVectors(glm::vec3 &V1, glm::vec3 &V2)
{
	float dot = glm::dot(V1, V2);
	float V1L1 = glm::length(V1);
	float V2L2 = glm::length(V2);

	float angle = acos((dot / (V1L1 * V2L2)));

	angle = glm::degrees(angle);

	return angle;
}

// Update method runs repeatedly with the Render method
void Game::Update() 
{
	// increment the distance by a fixed amount
	m_currentDistance += m_dt * m_cameraSpeed;

	if (m_currentDistance > 900)
		return;

	camera_position = glm::vec3();
	// determine a point on the centreline, at a distance of m_currentDistance
	m_pCatmullRom->Sample(m_currentDistance, camera_position);


	//player_position
	// determine player_position on the centreline, at a distance of m_currentDistance +1 
	m_pCatmullRom->Sample(m_currentDistance + 8, player_position);

	//tangentCamera
	//a normalised tangent vector T that points from p to pNext
	glm::vec3 tangentCamera = glm::normalize(player_position - camera_position);


	// determine a point on the centreline
	m_pCatmullRom->Sample(m_currentDistance + 9, player_lookAt);

	//a normalised tangent vector T that points from p to pNext
	glm::vec3 T = glm::normalize(player_lookAt- player_position);
	glm::vec3 N = glm::normalize(glm::cross(T, glm::vec3(0, 1, 0)));
	glm::vec3 B = glm::normalize(glm::cross(N, T));

	player_orientation = glm::mat4(glm::mat3(T, B, N));

	//m_playerCurrentDistance += m_dt * 0.001;
	//glm::vec3 desiredPlayerDir = player_lookAt - player_position;
	//lookAtAngle = AngleBetweenVectors(desiredPlayerDir, curPlayerDir);

	debug = int(m_currentDistance);

	playerTf = glm::mat4();
	//playerTf = glm::lookAt(player_position, player_position+tmp, camera_upVector);
	//playerTf = glm::translate(playerTf, glm::vec3(0, 0, 100));
	
	if (GetKeyState(VK_RIGHT) & 0x80 || GetKeyState('D') & 0x80)
		playerTOffset += 0.01f*m_dt;

	if (GetKeyState(VK_LEFT) & 0x80 || GetKeyState('A') & 0x80)
		playerTOffset -= 0.01f*m_dt;

	playerTOffset = glm::clamp(playerTOffset, -2.0f, 2.0f);

	glm::vec3 offsetTVec = glm::vec3(N.x*playerTOffset, N.y*playerTOffset, N.z*playerTOffset);
	player_position = player_position + offsetTVec;

	playerTf = glm::translate(player_position);
	playerTf = glm::scale(playerTf, glm::vec3(0.5f, 0.5f, 0.5f));
	playerTf *= player_orientation;

	//playerTf = glm::rotate(playerTf,lookAtAngle, glm::vec3(0, 1, 0));

	glm::vec4 vec(1.0f, 0.0f, 0.0f, 1.0f);

	for (size_t i = 0; i < ringCout; i++)
	{
		glm::vec4 translationColumn = obstacleTf[i] * vec;
		glm::vec3 obstacle_position = glm::vec3(translationColumn.x, translationColumn.y, translationColumn.z);
		float player_distance = glm::length(obstacle_position - player_position);
		if (player_distance < 2)
		{
			scores++;
		    obstacleTf[i] = glm::mat4();
		}
	}


	//yOffset of camera
	glm::vec3 yOffset = glm::vec3(0, 3, 0);
	//Follow camera to player
	m_pCamera->Set(camera_position + yOffset, camera_position + 20.0f * tangentCamera, camera_upVector);

	//Set camera at top View
	//m_pCamera->Set(glm::vec3(-140.0f, 15.0f, 0.0f), glm::vec3(-140.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f));

	//Contoll camera with mouse and keyboard
	//Update the camera using the amount of time that has elapsed to avoid framerate dependent motion
	//m_pCamera->Update(m_dt);

	m_pAudio->Update();
}

void Game::UI()
{
	CShaderProgram *fontProgram = (*m_pShaderPrograms)[1];

	RECT dimensions = m_gameWindow.GetDimensions();
	int height = dimensions.bottom - dimensions.top;

	fontProgram->UseProgram();
	glDisable(GL_DEPTH_TEST);
	fontProgram->SetUniform("matrices.modelViewMatrix", glm::mat4(1));
	fontProgram->SetUniform("matrices.projMatrix", m_pCamera->GetOrthographicProjectionMatrix());
	fontProgram->SetUniform("vColour", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
	m_pFtFont->Render(20, height - 80, 20, "Score: %d", scores);
}

void Game::DisplayFrameRate()
{
	CShaderProgram *fontProgram = (*m_pShaderPrograms)[1];

	RECT dimensions = m_gameWindow.GetDimensions();
	int height = dimensions.bottom - dimensions.top;

	// Increase the elapsed time and frame counter
	m_elapsedTime += m_dt;
	m_frameCount++;

	// Now we want to subtract the current time by the last time that was stored
	// to see if the time elapsed has been over a second, which means we found our FPS.
	if (m_elapsedTime > 1000)
    {
		m_elapsedTime = 0;
		m_framesPerSecond = m_frameCount;

		// Reset the frames per second
		m_frameCount = 0;
    }

	if (m_framesPerSecond > 0) 
	{
		// Use the font shader program and render the text
		fontProgram->UseProgram();
		glDisable(GL_DEPTH_TEST);
		fontProgram->SetUniform("matrices.modelViewMatrix", glm::mat4(1));
		fontProgram->SetUniform("matrices.projMatrix", m_pCamera->GetOrthographicProjectionMatrix());
		fontProgram->SetUniform("vColour", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
		m_pFtFont->Render(20, height - 20, 20, "FPS: %d", m_framesPerSecond);
	}
}

// The game loop runs repeatedly until game over
void Game::GameLoop()
{
	// Fixed timer
	m_dt = m_pHighResolutionTimer->Elapsed();
	if (m_dt > 1000.0 / (double) Game::FPS) 
	{
		m_pHighResolutionTimer->Start();
		Update();
		Render();
	}
	
	// Variable timer
	/*m_pHighResolutionTimer->Start();
	Update();
	Render();
	m_dt = m_pHighResolutionTimer->Elapsed();*/
}

WPARAM Game::Execute() 
{
	m_pHighResolutionTimer = new CHighResolutionTimer;
	m_gameWindow.Init(m_hInstance);

	if(!m_gameWindow.Hdc()) {
		return 1;
	}

	Initialise();

	m_pHighResolutionTimer->Start();

	
	MSG msg;

	while(1) {													
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) { 
			if(msg.message == WM_QUIT) {
				break;
			}

			TranslateMessage(&msg);	
			DispatchMessage(&msg);
		} else if (m_appActive) {
			GameLoop();
		} 
		else Sleep(200); // Do not consume processor power if application isn't active
	}

	m_gameWindow.Deinit();

	return(msg.wParam);
}

LRESULT Game::ProcessEvents(HWND window,UINT message, WPARAM w_param, LPARAM l_param) 
{
	LRESULT result = 0;

	switch (message) 
	{
	case WM_ACTIVATE:
	{
		switch(LOWORD(w_param))
		{
			case WA_ACTIVE:
			case WA_CLICKACTIVE:
				m_appActive = true;
				m_pHighResolutionTimer->Start();
				break;
			case WA_INACTIVE:
				m_appActive = false;
				break;
		}
		break;
		}

	case WM_SIZE:
			RECT dimensions;
			GetClientRect(window, &dimensions);
			m_gameWindow.SetDimensions(dimensions);
		break;

	case WM_PAINT:
		PAINTSTRUCT ps;
		BeginPaint(window, &ps);
		EndPaint(window, &ps);
		break;

	case WM_KEYDOWN:
		switch(w_param) 
		{
			case VK_ESCAPE:
				PostQuitMessage(0);
				break;
			case '1':
				m_pAudio->PlayEventSound();
				break;
			case VK_F1:
				m_pAudio->PlayEventSound();
				break;
		}
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		result = DefWindowProc(window, message, w_param, l_param);
		break;
	}

	return result;
}

Game& Game::GetInstance() 
{
	static Game instance;

	return instance;
}

void Game::SetHinstance(HINSTANCE hinstance) 
{
	m_hInstance = hinstance;
}

LRESULT CALLBACK WinProc(HWND window, UINT message, WPARAM w_param, LPARAM l_param)
{
	return Game::GetInstance().ProcessEvents(window, message, w_param, l_param);
}

int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE, PSTR, int) 
{
	Game &game = Game::GetInstance();
	game.SetHinstance(hinstance);

	return game.Execute();
}
