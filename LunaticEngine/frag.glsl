#version 460 core

out vec3 FragColor;

//in vec2 fragTexCoord;

//uniform sampler2D u_texture;
//uniform bool u_hasTexture;

uniform vec3 u_color;

void main() {
	FragColor = u_color;

	/*if (!u_hasTexture) {
		FragColor = u_color;
		return;
	}

	vec4 colour = texture(u_texture, j_fragTexCoord);

	if (colour.a < 0.01) {
		discard;
	}

	FragColor = vec3(colour.r, colour.g, colour.b);*/
}