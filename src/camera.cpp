/******************************************************************************
* Empty Clip
* Copyright (C) 2022  Alan Witkowski
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*******************************************************************************/
#include <camera.h>
#include <graphics.h>
#include <constants.h>
#include <opengl.h>
#include <ui/ui.h>
#include <glm/gtx/norm.hpp>

// Initialize
_Camera::_Camera(const glm::vec3 &Position, float UpdateDivisor) :
	AABB(0),
	LastPosition(Position),
	Position(Position),
	TargetPosition(Position),
	UpdateDivisor(UpdateDivisor),
	Frustum(0),
	Fovy(CAMERA_FOVY),
	Near(CAMERA_NEAR),
	Far(CAMERA_FAR) {
}

// Shutdown
_Camera::~_Camera() {
}

void _Camera::CalculateFrustum(float AspectRatio) {
	Frustum.y = (float)std::tan(Fovy / 360 * glm::pi<float>()) * Near;
	Frustum.x = Frustum.y * AspectRatio;
	Projection = glm::frustum(-Frustum.x, Frustum.x, Frustum.y, -Frustum.y, Near, Far);
}

// Set up 3d projection matrix
void _Camera::Set3DProjection(double BlendFactor) {
	glm::vec3 DrawPosition;
	GetDrawPosition(BlendFactor, DrawPosition);

	glm::vec2 Size(DrawPosition.z * Graphics.AspectRatio, DrawPosition.z);

	// Get AABB at z=0
	AABB[0] = -Size.x + DrawPosition.x;
	AABB[1] = -Size.y + DrawPosition.y;
	AABB[2] = Size.x + DrawPosition.x;
	AABB[3] = Size.y + DrawPosition.y;

	Transform = Projection * glm::translate(glm::mat4(1.0f), -DrawPosition);
}

// Converts screen space to world space
void _Camera::ConvertScreenToWorld(const glm::ivec2 &Point, glm::vec2 &WorldPosition) {
	WorldPosition.x = (Point.x / (float)(Graphics.ViewportSize.x) - 0.5f) * Position.z * Graphics.AspectRatio * 2 + Position.x;
	WorldPosition.y = (Point.y / (float)(Graphics.ViewportSize.y) - 0.5f) * Position.z * 2 + Position.y;
}

// Converts world space to screen space
void _Camera::ConvertWorldToScreen(const glm::vec2 &WorldPosition, glm::ivec2 &Point) {
	Point.x = Graphics.ViewportSize.x * (0.5f + ((WorldPosition.x - Position.x) / (Position.z * Graphics.AspectRatio * 2)));
	Point.y = Graphics.ViewportSize.y * (0.5f + ((WorldPosition.y - Position.y) / (Position.z * 2)));
}

// Update camera
void _Camera::Update(double FrameTime) {
	LastPosition = Position;

	// Cap distance
	if(TargetPosition.z <= 1.0f)
		TargetPosition.z = 1.0f;
	else if(TargetPosition.z >= Far)
		TargetPosition.z = Far;

	// Update position
	glm::vec2 Delta(TargetPosition - Position);
	if(std::abs(Delta.x) > 0.005f)
		Position.x += Delta.x / UpdateDivisor;
	else
		Position.x = TargetPosition.x;

	if(std::abs(Delta.y) > 0.005f)
		Position.y += Delta.y / UpdateDivisor;
	else
		Position.y = TargetPosition.y;

	// Update distance
	float DeltaZ = TargetPosition.z - Position.z;
	if(std::abs(DeltaZ) > 0.01f)
		Position.z += DeltaZ / UpdateDivisor;
	else
		Position.z = TargetPosition.z;
}

// Interpolate between position and last position
void _Camera::GetDrawPosition(double BlendFactor, glm::vec3 &DrawPosition) {
	if(Position == LastPosition)
		DrawPosition = Position;
	else
		DrawPosition = Position * (float)BlendFactor + LastPosition * (1.0f - (float)BlendFactor);
}

// Determines whether a circle is in view
bool _Camera::IsCircleInView(const glm::vec2 &Center, float Radius) const {

	// Get closest point on AABB
	glm::vec2 Point(Center);
	if(Point.x < AABB[0])
		Point.x = AABB[0];
	if(Point.y < AABB[1])
		Point.y = AABB[1];
	if(Point.x > AABB[2])
		Point.x = AABB[2];
	if(Point.y > AABB[3])
		Point.y = AABB[3];

	// Test circle collision with point
	float DistanceSquared = glm::distance2(Point, Center);
	bool Hit = DistanceSquared < Radius * Radius;

	return Hit;
}

// Determines whether an AABB is in view
bool _Camera::IsAABBInView(const float *Bounds) const {

	if(Bounds[2] < AABB[0] || Bounds[0] > AABB[2])
		return false;
	if(Bounds[3] < AABB[1] || Bounds[1] > AABB[3])
		return false;

	return true;
}
