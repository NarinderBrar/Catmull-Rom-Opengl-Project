#pragma once
#include "./include/glm/gtc/type_ptr.hpp"
#include "./include/glm/gtc/matrix_transform.hpp"


class PlayerTransform
{
public:
	PlayerTransform();
	~PlayerTransform();

	glm::vec3 GetPosition() const;
	glm::vec3 GetView() const;	
	glm::vec3 GetUpVector() const;

	glm::vec3 GetStrafeVector() const;
	glm::mat4* GetPerspectiveProjectionMatrix();
	glm::mat4* GetOrthographicProjectionMatrix();
	glm::mat4 GetViewMatrix();

	void Set(glm::vec3 &position, glm::vec3 &viewpoint, glm::vec3 &upVector);

	void RotateViewPoint(float angle, glm::vec3 &viewPoint);

	// Strafe the camera (move it side to side)
	void Strafe(double direction);

	// Advance the camera (move it forward or backward)
	void Advance(double direction);

	// Update the camera
	void Update(double dt);

	// Set the projection matrices
	void SetPerspectiveProjectionMatrix(float fov, float aspectRatio, float nearClippingPlane, float farClippingPlane);
	void SetOrthographicProjectionMatrix(int width, int height);

	glm::mat3 ComputeNormalMatrix(const glm::mat4 &modelViewMatrix);

private:
	glm::vec3 m_position;			// The position of the camera's centre of projection
	glm::vec3 m_view;				// The camera's viewpoint (point where the camera is looking)
	glm::vec3 m_upVector;			// The camera's up vector
	glm::vec3 m_strafeVector;		// The camera's strafe vector

	float m_speed;					// How fast the camera moves

	glm::mat4 m_perspectiveProjectionMatrix;		// Perspective projection matrix
	glm::mat4 m_orthographicProjectionMatrix;		// Orthographic projection matrix
};

