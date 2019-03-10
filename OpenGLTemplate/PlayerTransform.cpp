#include "PlayerTransform.h"
#include "gamewindow.h"

PlayerTransform::PlayerTransform()
{
	m_position = glm::vec3(0.0f, 10.0f, 100.0f);//0,10,100
	m_view = glm::vec3(0.0f, 0.0f, 0.0f);//0,0,0
	m_upVector = glm::vec3(0.0f, 1.0f, 0.0f);
	m_speed = 0.025f;
}

PlayerTransform::~PlayerTransform()
{

}

void PlayerTransform::Set(glm::vec3 &position, glm::vec3 &viewpoint, glm::vec3 &upVector)
{
	m_position = position;
	m_view = viewpoint;
	m_upVector = upVector;
}

void PlayerTransform::RotateViewPoint(float fAngle, glm::vec3 &vPoint)
{
	glm::vec3 vView = m_view - m_position;

	glm::mat4 R = glm::rotate(glm::mat4(1), fAngle * 180.0f / (float)M_PI, vPoint);
	glm::vec4 newView = R * glm::vec4(vView, 1);

	m_view = m_position + glm::vec3(newView);
}

void PlayerTransform::Strafe(double direction)
{
	float speed = (float)(m_speed * direction);

	m_position.x = m_position.x + m_strafeVector.x * speed;
	m_position.z = m_position.z + m_strafeVector.z * speed;

	m_view.x = m_view.x + m_strafeVector.x * speed;
	m_view.z = m_view.z + m_strafeVector.z * speed;
}

void PlayerTransform::Advance(double direction)
{
	float speed = (float)(m_speed * direction);

	glm::vec3 view = glm::normalize(m_view - m_position);
	m_position = m_position + view * speed;
	m_view = m_view + view * speed;
}

void PlayerTransform::Update(double dt)
{
	glm::vec3 vector = glm::cross(m_view - m_position, m_upVector);
	m_strafeVector = glm::normalize(vector);
}

glm::vec3 PlayerTransform::GetPosition() const
{
	return m_position;
}

glm::vec3 PlayerTransform::GetView() const
{
	return m_view;
}

glm::vec3 PlayerTransform::GetUpVector() const
{
	return m_upVector;
}

glm::vec3 PlayerTransform::GetStrafeVector() const
{
	return m_strafeVector;
}

glm::mat4* PlayerTransform::GetPerspectiveProjectionMatrix()
{
	return &m_perspectiveProjectionMatrix;
}

glm::mat4* PlayerTransform::GetOrthographicProjectionMatrix()
{
	return &m_orthographicProjectionMatrix;
}

glm::mat4 PlayerTransform::GetViewMatrix()
{
	return glm::lookAt(m_position, m_view, m_upVector);
}

glm::mat3 PlayerTransform::ComputeNormalMatrix(const glm::mat4 &modelViewMatrix)
{
	return glm::transpose(glm::inverse(glm::mat3(modelViewMatrix)));
}