#version 120
#extension GL_ARB_texture_rectangle : enable
#extension GL_EXT_gpu_shader4 : enable

uniform sampler2DRect Front;
uniform sampler2DRect Back;
uniform sampler2DRect mask;

void main(){
	/********************
	********************/
	vec2 tex_pos = gl_TexCoord[0].xy;
	
	vec4 col_mask	= texture2DRect(mask, tex_pos);
	vec4 col_Front	= texture2DRect(Front, tex_pos);
	vec4 col_Back	= texture2DRect(Back, tex_pos);
	
	/*
	vec3 rgb2lum = vec3(0.299, 0.587, 0.114);
	
	// col_Back.rgb *= 0.4;
	// col_Back.gb = vec2(0.0, 0.0);
	float val = dot(rgb2lum, col_Back.rgb);
	col_Back.rgb = vec3(val, val, val);
	col_Back.rgb *= 0.4;
	*/
	
	/********************
	********************/
	vec4 color = (0.0 < col_mask.r + col_mask.g + col_mask.b) ? col_Front : col_Back;
	color.a = 1.0;
	
	/********************
	********************/
	gl_FragColor = color;
}
