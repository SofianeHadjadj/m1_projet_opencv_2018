#version 330
uniform sampler2D myTexture;
uniform vec4 color;
uniform int test;

in  vec2 vsoTexCoord;
out vec4 fragColor;

void main(void) {
  if(test == 0) {
    fragColor = color;
  } else {
    fragColor = texture(myTexture, vsoTexCoord.st);
  }
}
