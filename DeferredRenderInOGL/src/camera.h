/*
 camera.h
 OpenGL Camera Code
 Capable of 2 modes, orthogonal, and free
 Quaternion camera code adapted from: http://hamelot.co.uk/visualization/opengl-camera/
 Written by Hammad Mazhar
 */
#ifndef CAMERA_H
#define CAMERA_H

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/freeglut.h>
#endif

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>

enum CameraType {
	ORTHO, FREE
};
enum CameraDirection {
	UP, DOWN, LEFT, RIGHT, FORWARD, BACK
};

class Camera {
	public:
		Camera();
		~Camera();

		void Reset();
		//This function updates the camera
		//Depending on the current camera mode, the projection and viewport matricies are computed
		//Then the position and location of the camera is updated
		void Update();

		//Given a specific moving direction, the camera will be moved in the appropriate direction
		//For a spherical camera this will be around the look_at point
		//For a free camera a delta will be computed for the direction of movement.
		void Move(CameraDirection dir);
		//Change the pitch (up, down) for the free camera
		void ChangePitch(float degrees);
		//Change heading (left, right) for the free camera
		void ChangeHeading(float degrees);

		//Change the heading and pitch of the camera based on the 2d movement of the mouse
		void Move2D(int x, int y);

		//Setting Functions
		//Changes the camera mode, only three valid modes, Ortho, Free, and Spherical
		void SetMode(CameraType cam_mode);
		//Set the position of the camera
		void SetPosition(glm::vec3 pos);
		//Set's the look at point for the camera
		void SetLookAt(glm::vec3 pos);
		//Changes the Field of View (FOV) for the camera
		void SetFOV(double fov);
		//Change the viewport location and size
		void SetViewport(int loc_x, int loc_y, int width, int height);
		//Change the clipping distance for the camera
		void SetClipping(double near_clip_distance, double far_clip_distance);

		void SetDistance(double cam_dist);
		void SetPos(int button, int state, int x, int y);

		//Getting Functions
		CameraType GetMode();
		void GetViewport(int &loc_x, int &loc_y, int &width, int &height);
		void GetMatricies(glm::mat4 &P, glm::mat4 &V, glm::mat4 &M);

		CameraType camera_mode;

		int viewport_x;
		int viewport_y;

		int window_width;
		int window_height;

		double aspect;
		double field_of_view;
		double near_clip;
		double far_clip;

		float camera_scale;
		float camera_heading;
		float camera_pitch;

		float max_pitch_rate;
		float max_heading_rate;
		bool move_camera;

		glm::vec3 camera_position;
		glm::vec3 camera_position_delta;
		glm::vec3 camera_look_at;
		glm::vec3 camera_direction;

		glm::vec3 camera_up;
		glm::vec3 mouse_position;

		glm::mat4 projection;
		glm::mat4 view;
		glm::mat4 model;
		glm::mat4 MVP;

};

class CameraManager
{
public:
    static Camera& getCamera(std::string);
};
#endif