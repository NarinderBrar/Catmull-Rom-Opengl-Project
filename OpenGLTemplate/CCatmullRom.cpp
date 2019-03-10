#define _USE_MATH_DEFINES
#include <math.h>
#include "CCatmullRom.h"
#include <iostream>

CCatmullRom::CCatmullRom()
{
	m_vertexCount = 0;
	m_currentDistance = 0.0f;
}

CCatmullRom::~CCatmullRom()
{}

// Perform Catmull Rom spline interpolation between four points, interpolating the space between p1 and p2
glm::vec3 CCatmullRom::Interpolate(glm::vec3 &p0, glm::vec3 &p1, glm::vec3 &p2, glm::vec3 &p3, float t)
{
	float t2 = t * t;
	float t3 = t2 * t;

	glm::vec3 a = p1;
	glm::vec3 b = 0.5f * (-p0 + p2);
	glm::vec3 c = 0.5f * (2.0f*p0 - 5.0f*p1 + 4.0f*p2 - p3);
	glm::vec3 d = 0.5f * (-p0 + 3.0f*p1 - 3.0f*p2 + p3);

	return a + b * t + c * t2 + d * t3;
}

void CCatmullRom::SetControlPoints()
{
	// Set control points (m_controlPoints) here, or load from disk
	// Optionally, set upvectors (m_controlUpVectors, one for each control point as well)

	m_controlPoints.push_back(glm::vec3(240, 0, 114));
	m_controlPoints.push_back(glm::vec3(80, 0, 70));
	m_controlPoints.push_back(glm::vec3(-21, 0, 74));

	m_controlPoints.push_back(glm::vec3(-110, 0, 60));
	m_controlPoints.push_back(glm::vec3(-147, 0, -82));
	m_controlPoints.push_back(glm::vec3(0, 0, -143));

	m_controlPoints.push_back(glm::vec3(109, 0, -183));
	m_controlPoints.push_back(glm::vec3(218, 0, -223));
}

// Determine lengths along the control points, which is the set of control points forming the closed curve
void CCatmullRom::ComputeLengthsAlongControlPoints()
{
	int M = (int)m_controlPoints.size();

	float fAccumulatedLength = 0.0f;
	m_distances.push_back(fAccumulatedLength);
	for (int i = 1; i < M; i++) {
		fAccumulatedLength += glm::distance(m_controlPoints[i - 1], m_controlPoints[i]);
		m_distances.push_back(fAccumulatedLength);
	}

	// Get the distance from the last point to the first
	//fAccumulatedLength += glm::distance(m_controlPoints[M - 1], m_controlPoints[0]);
	//m_distances.push_back(fAccumulatedLength);
}

// Return the point (and upvector, if control upvectors provided) based on a distance d along the control polygon
bool CCatmullRom::Sample(float d, glm::vec3 &p, glm::vec3 &up)
{
	if (d < 0)
		return false;

	int M = (int)m_controlPoints.size();
	if (M == 0)
		return false;


	float fTotalLength = m_distances[m_distances.size() - 1];

	// The the current length along the control polygon; handle the case where we've looped around the track
	float fLength = d - (int)(d / fTotalLength) * fTotalLength;

	// Find the current segment
	int j = -1;
	for (int i = 0; i < (int)m_distances.size(); i++) {
		if (fLength >= m_distances[i] && fLength < m_distances[i + 1]) {
			j = i; // found it!
			break;
		}
	}

	if (j == -1)
		return false;

	// Interpolate on current segment -- get t
	float fSegmentLength = m_distances[j + 1] - m_distances[j];
	float t = (fLength - m_distances[j]) / fSegmentLength;

	// Get the indices of the four points along the control polygon for the current segment
	int iPrev = ((j - 1) + M) % M;
	int iCur = j;
	int iNext = (j + 1) % M;
	int iNextNext = (j + 2) % M;

	// Interpolate to get the point (and upvector)
	p = Interpolate(m_controlPoints[iPrev], m_controlPoints[iCur], m_controlPoints[iNext], m_controlPoints[iNextNext], t);
	if (m_controlUpVectors.size() == m_controlPoints.size())
		up = glm::normalize(Interpolate(m_controlUpVectors[iPrev], m_controlUpVectors[iCur], m_controlUpVectors[iNext], m_controlUpVectors[iNextNext], t));

	return true;
}

// Sample a set of control points using an open Catmull-Rom spline, to produce a set of iNumSamples that are (roughly) equally spaced
void CCatmullRom::UniformlySampleControlPoints(int numSamples)
{
	glm::vec3 p, up;

	// Compute the lengths of each segment along the control polygon, and the total length
	ComputeLengthsAlongControlPoints();
	float fTotalLength = m_distances[m_distances.size() - 1];

	// The spacing will be based on the control polygon
	float fSpacing = fTotalLength / numSamples;

	// Call PointAt to sample the spline, to generate the points
	for (int i = 0; i < numSamples; i++) 
	{
		Sample(i * fSpacing, p, up);
		m_centrelinePoints.push_back(p);
		if (m_controlUpVectors.size() > 0)
			m_centrelineUpVectors.push_back(up);
	}

	// Repeat once more for truly equidistant points
	m_controlPoints = m_centrelinePoints;
	m_controlUpVectors = m_centrelineUpVectors;
	m_centrelinePoints.clear();
	m_centrelineUpVectors.clear();
	m_distances.clear();
	ComputeLengthsAlongControlPoints();
	fTotalLength = m_distances[m_distances.size() - 1];
	fSpacing = fTotalLength / numSamples;
	for (int i = 0; i < numSamples; i++) {
		Sample(i * fSpacing, p, up);
		m_centrelinePoints.push_back(p);
		if (m_controlUpVectors.size() > 0)
			m_centrelineUpVectors.push_back(up);
	}
}

void CCatmullRom::CreateCentreline()
{
	// Call Set Control Points
	SetControlPoints();

	// Call UniformlySampleControlPoints with the number of samples required
	UniformlySampleControlPoints(500);

	// Create a VAO called m_vaoCentreline and a VBO to get the points onto the graphics card

	// Use VAO to store state associated with vertices
	glGenVertexArrays(1, &m_vaoCentreline);
	glBindVertexArray(m_vaoCentreline);

	// Create a VBO
	CVertexBufferObject vbo;
	vbo.Create();
	vbo.Bind();

	glm::vec2 texCoord(10.0f, 10.0f);
	glm::vec3 normal(0.0f, 1.0f, 0.0f);

	int M = (int)m_controlPoints.size();

	for (int i = 0; i < M; i++) 
	{
		glm::vec3 v =  m_controlPoints[i];
		vbo.AddData(&v, sizeof(glm::vec3));
		vbo.AddData(&texCoord, sizeof(glm::vec2));
		vbo.AddData(&normal, sizeof(glm::vec3));
	}

	// Upload the VBO to the GPU
	vbo.UploadDataToGPU(GL_STATIC_DRAW);

	// Set the vertex attribute locations
	GLsizei stride = 2 * sizeof(glm::vec3) + sizeof(glm::vec2);

	// Vertex positions
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, 0);

	// Texture coordinates
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)sizeof(glm::vec3));

	// Normal vectors
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(glm::vec3)+ sizeof(glm::vec2)));
}

void CCatmullRom::CreateOffsetCurves()
{
	// Compute the offset curves, one left, and one right.  Store the points in m_leftOffsetPoints and m_rightOffsetPoints respectively

	// Generate two VAOs called m_vaoLeftOffsetCurve and m_vaoRightOffsetCurve, each with a VBO, and get the offset curve points on the graphics card
	// Note it is possible to only use one VAO / VBO with all the points instead.
	int M = (int)m_centrelinePoints.size();

	for (int i = 0; i < M-1; i++)
	{
		//a normalised tangent vector T that points from p to pNext
		glm::vec3 tangent = glm::normalize(m_centrelinePoints[i + 1] - m_centrelinePoints[i]);
		glm::vec3 y = glm::vec3(0,1,0);
		glm::vec3 normal = glm::cross(tangent, y);

		m_leftOffsetPoints.push_back(m_centrelinePoints[i] - ((m_pathWidth / 2) * normal));
		m_rightOffsetPoints.push_back(m_centrelinePoints[i] + ((m_pathWidth / 2) * normal));

		std::cout << glm::to_string(m_leftOffsetPoints[i]) << std::endl;
	}

	// Use VAO to store state associated with vertices
	glGenVertexArrays(1, &m_vaoLeftOffsetCurve);
	glBindVertexArray(m_vaoLeftOffsetCurve);

	// Create a VBOL
	CVertexBufferObject vbol;
	vbol.Create();
	vbol.Bind();

	glm::vec2 texCoordl(0.0f, 0.0f);
	glm::vec3 normall(0.0f, 1.0f, 0.0f);

	int Ml = (int)m_leftOffsetPoints.size();

	for (int i = 0; i < Ml; i++)
	{
		glm::vec3 vl = m_leftOffsetPoints[i];
		vbol.AddData(&vl, sizeof(glm::vec3));
		vbol.AddData(&texCoordl, sizeof(glm::vec2));
		vbol.AddData(&normall, sizeof(glm::vec3));
	}

	// Upload the VBO to the GPU
	vbol.UploadDataToGPU(GL_STATIC_DRAW);

	// Set the vertex attribute locations
	GLsizei stridel = 2 * sizeof(glm::vec3) + sizeof(glm::vec2);

	// Vertex positions
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stridel, 0);

	// Texture coordinates
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stridel, (void*)sizeof(glm::vec3));

	// Normal vectors
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stridel, (void*)(sizeof(glm::vec3) + sizeof(glm::vec2)));

	// Use VAO to store state associated with vertices
	glGenVertexArrays(1, &m_vaoRightOffsetCurve);
	glBindVertexArray(m_vaoRightOffsetCurve);

	// Create a VBOL
	CVertexBufferObject vbor;
	vbor.Create();
	vbor.Bind();

	glm::vec2 texCoordr(0.0f, 0.0f);
	glm::vec3 normalr(0.0f, 1.0f, 0.0f);

	int Mr = (int)m_rightOffsetPoints.size();

	for (int i = 0; i < Mr; i++)
	{
		glm::vec3 vr = m_rightOffsetPoints[i];
		vbor.AddData(&vr, sizeof(glm::vec3));
		vbor.AddData(&texCoordr, sizeof(glm::vec2));
		vbor.AddData(&normalr, sizeof(glm::vec3));
	}

	// Upload the VBO to the GPU
	vbor.UploadDataToGPU(GL_STATIC_DRAW);

	// Set the vertex attribute locations
	GLsizei stride_r = 2 * sizeof(glm::vec3) + sizeof(glm::vec2);

	// Vertex positions
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride_r, 0);

	// Texture coordinates
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride_r, (void*)sizeof(glm::vec3));

	// Normal vectors
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride_r, (void*)(sizeof(glm::vec3) + sizeof(glm::vec2)));
}

void CCatmullRom::CreateTrack()
{
	// Load the texture
	m_texture.Load("resources\\textures\\road.jpg", true);

	// Set parameters for texturing using sampler object
	m_texture.SetSamplerObjectParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	m_texture.SetSamplerObjectParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	m_texture.SetSamplerObjectParameter(GL_TEXTURE_WRAP_S, GL_REPEAT);
	m_texture.SetSamplerObjectParameter(GL_TEXTURE_WRAP_T, GL_REPEAT);

	// Generate a VAO called m_vaoTrack and a VBO to get the offset curve points and indices on the graphics card

	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	// Use VAO to store state associated with vertices
	glGenVertexArrays(1, &m_vaoTrack);
	glBindVertexArray(m_vaoTrack);

	// Create a VBO
	CVertexBufferObject vaoTrack;
	vaoTrack.Create();
	vaoTrack.Bind();

	glm::vec2 texCoord00(0.0f, 0.0f);
	glm::vec2 texCoord01(0.0f, 1);
	glm::vec2 texCoord10(0.1f, 0);
	glm::vec2 texCoord11(0.1f, 1);

	m_vertexCount = (int)m_rightOffsetPoints.size() - m_pathVertexOffset;

	for (int i = 0; i < m_vertexCount; i+= m_pathVertexOffset)
	{
		m_pathPoints.push_back(m_leftOffsetPoints[i]);//490
		m_pathUV.push_back(texCoord00);

		m_pathPoints.push_back(m_rightOffsetPoints[i]);//490
		m_pathUV.push_back(texCoord01);

		m_pathPoints.push_back(m_leftOffsetPoints[i + m_pathVertexOffset]);//500
		m_pathUV.push_back(texCoord10);

		m_pathPoints.push_back(m_leftOffsetPoints[i + m_pathVertexOffset]);//500
		m_pathUV.push_back(texCoord10);

		m_pathPoints.push_back(m_rightOffsetPoints[i]);//490
		m_pathUV.push_back(texCoord01);

		m_pathPoints.push_back(m_rightOffsetPoints[i + m_pathVertexOffset]);//500
		m_pathUV.push_back(texCoord11);
	}

	//For connecting loop
	/*m_pathPoints.push_back(m_leftOffsetPoints[m_vertexCount]);
	m_pathPoints.push_back(m_rightOffsetPoints[m_vertexCount]);
	m_pathPoints.push_back(m_leftOffsetPoints[0]);

	m_pathPoints.push_back(m_leftOffsetPoints[0]);
	m_pathPoints.push_back(m_rightOffsetPoints[m_vertexCount]);
	m_pathPoints.push_back(m_rightOffsetPoints[0]);

	m_pathPoints.push_back(m_leftOffsetPoints[0]);
	m_pathPoints.push_back(m_rightOffsetPoints[0]);
	m_pathPoints.push_back(m_leftOffsetPoints[m_pathVertexOffset]);*/

	m_vertexCount = (int)m_pathPoints.size();

	glm::vec3 normal(0.0f, 1.0f, 0.0f);

	// Put the vertex attributes in the VBO
	for (unsigned int i = 0; i < m_vertexCount; i++)
	{
		vaoTrack.AddData(&m_pathPoints[i], sizeof(glm::vec3));
		vaoTrack.AddData(&m_pathUV[i], sizeof(glm::vec2));
		vaoTrack.AddData(&normal, sizeof(glm::vec3));
	}

	// Upload the VBO to the GPU
	vaoTrack.UploadDataToGPU(GL_STATIC_DRAW);

	// Set the vertex attribute locations
	GLsizei istride = 2 * sizeof(glm::vec3) + sizeof(glm::vec2);

	// Vertex positions
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, istride, 0);
	// Texture coordinates
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, istride, (void*)sizeof(glm::vec3));
	// Normal vectors
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, istride, (void*)(sizeof(glm::vec3) + sizeof(glm::vec2)));
}

void CCatmullRom::RenderCentreline()
{
	// Bind the VAO m_vaoCentreline and render it
	glBindVertexArray(m_vaoCentreline);

	int M = (int)m_controlPoints.size();
	glLineWidth(5);
	glDrawArrays(GL_POINTS, 0, M);
}

void CCatmullRom::RenderOffsetCurves()
{
	// Bind the VAO m_vaoLeftOffsetCurve and render it
	glBindVertexArray(m_vaoLeftOffsetCurve);

	int Ml = (int)m_leftOffsetPoints.size();
	glPointSize(2.0f);
	glDrawArrays(GL_POINTS, 0, Ml);

	// Bind the VAO m_vaoRightOffsetCurve and render it
	glBindVertexArray(m_vaoRightOffsetCurve);

	int Mr = (int)m_rightOffsetPoints.size();
	glPointSize(2.0f);
	glDrawArrays(GL_POINTS, 0, Ml);
}

void CCatmullRom::RenderTrack()
{
	// Bind the VAO m_vaoTrack and render it
	glBindVertexArray(m_vaoTrack);
	m_texture.Bind();
	glDrawArrays(GL_TRIANGLE_STRIP, 0, m_vertexCount);
}

int CCatmullRom::CurrentLap(float d)
{
	return (int)(d / m_distances.back());
}