#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include "Raycaster.h"
#include "v3math.h"

typedef struct Object {
  // kind 0 default, 1 camera, 2 sphere, 3 plane
  int kind;
  float diffuse[3];
  float specular[3];
  float position[3];
  float reflectivity;
  float ns;

  union {
    // different structs for different objects, like sphere/plane/camera/etc

    // struct for camera
    struct {
      // camera is assumed to be at position [0, 0, 0]
      float width;
      float height;
    };

    // struct for sphere
    struct {
      float radius;
    };

    // struct for plane
    struct {
      float normal[3];
    };
  };
} Object;

typedef struct Light {
  // kind 0 default, 1 point light, 2 spot light
  int kind;
  float position[3];
  float color[3];
  float theta;
  float spotlightDotProd;

  // point light
  float radial_a0;
  float radial_a1;
  float radial_a2;

  // spot light
  float angular_a0;
  float direction[3];
} Light;

void displayTime(clock_t time) {
  double total = ((double) time) / CLOCKS_PER_SEC;

  int milliseconds = (int) (-1 * (((int) total) - total) * 1000);
  int minutes = (int) (total / 60);
  int seconds = ((int) total) % 60;

  printf("The raytracer took %d minutes, %d seconds, and %d milliseconds to execute\n", minutes, seconds, milliseconds);
}

// return smaller positive t value or negative if neither intersections are positive
// return negative if no intersection
float sphere_intersect(float *Rd, float *pos, float *R0, float radius) {
  // compute A, B, and C
  // since the Rd is normalized, A = 1
  float A = 1.0;
  float B = 2 * ((Rd[0] * (R0[0] - pos[0])) + 
                 (Rd[1] * (R0[1] - pos[1])) + 
                 (Rd[2] * (R0[2] - pos[2])));
  float C = (((R0[0] - pos[0]) * (R0[0] - pos[0])) + 
             ((R0[1] - pos[1]) * (R0[1] - pos[1])) + 
             ((R0[2] - pos[2]) * (R0[2] - pos[2])) - 
             (radius * radius));

  float discrim = (B * B) - (4 * C);

  // printf("Ray: [%f, %f, %f]   Pos: [%f, %f, %f]   Rad: %f   B: %f   C: %f   discrim: %f\n", ray[0], ray[1], ray[2], pos[0], pos[1], pos[2], radius, B, C, discrim);

  // check discriminant for solutions
  if (discrim < 0) {
    return -1.0;
  }

  // calculate smallest t value
  float tVal0 = ((-1 * B) - sqrtf(discrim)) / 2;

  if (tVal0 >= 0) {
    return tVal0;
  }
  // otherwise return t1. If t1 is negative as well it will be handled by the intersect() function
  return ((-1 * B) + sqrtf(discrim)) / 2;
}

// return t value
// return negative if no intersection
float plane_intersect(float *planePosition, float *planeNormal, 
                        float *R0, float *Rd) {
    //a,b,c,d represent the plane vector
    // V0 = numerator of the t value equation
    // Vd = denominator of the t value equation
    float t_value;
    float Vd;
    float V0;
    float distance;

    float origin[3] = {0, 0, 0};

    distance = sqrtf(((planePosition[0]-origin[0]) * (planePosition[0]-origin[0])) +
                     ((planePosition[1]-origin[1]) * (planePosition[1]-origin[1])) +
                     ((planePosition[2]-origin[2]) * (planePosition[2]-origin[2])));

    V0 = -(v3_dot_product(planeNormal, R0) + distance);
    Vd = v3_dot_product(planeNormal, Rd);

    //ray is parallel to plane there for no intersection
    if(Vd == 0)
    {   
        //return negative value since t needs to be positive
        return -1;
    }

    t_value = V0/Vd;

    return t_value;
}

// returns closest t val and reassigns closest object index
float shoot(int *closestObjIndex, Object *objects, float *Rd, float *R0, int skipObjIndex) {
  // create min
  float minIntersect = 10000000;
  int minIndex = -1;

  for (int index = 0; index < 128; index += 1) {
    // skip over current object
    if (index == skipObjIndex) {
      continue;
    }

    // determine kind of object and break if 0 (default)
    Object *currentObj = &objects[index];

    if (currentObj->kind == 0) {
      break;
    }

    // get t value
    float tVal = -1;
    
    if (currentObj->kind == 2) {
      // sphere
      tVal = sphere_intersect(Rd, currentObj->position, R0, currentObj->radius);
    }
    else if (currentObj->kind == 3) {
      // plane
      tVal = plane_intersect(currentObj->position, currentObj->normal, R0, Rd);
    }

    // check that t is positive
    if (tVal > 0 && tVal < minIntersect) {
      minIntersect = tVal;
      minIndex = index;
    }
  }

  // get color of min if there is a min
  if (minIndex >= 0) {
    *closestObjIndex = minIndex;
    return minIntersect;
  }

  *closestObjIndex = -1;
  return -1;
}

// puts the final color after calculations into illuminate
void illuminate(float *finalColor, Object *objects, Light *lights, int currObjIndex, float *point, float *rayInit, int *reflectLimit) {
  // printf("%d [%f %f %f] [%f %f %f] %d\n", currObjIndex, point[0], point[1], point[2], rayInit[0], rayInit[1], rayInit[2], *reflectLimit);
  if (*reflectLimit <= 0) {
    return;
  }
  *reflectLimit -= 1;

  float lightsColor[3] = {0, 0, 0};

  for (int lightI = 0; lightI < 128; lightI += 1) {
    Light *currentLight = &lights[lightI];

    if (currentLight->kind == 0) {
      break;
    }

    float minIntersect = 10000000;
    float minIndex = -1;

    // calculate vector and Rd from point to light
    float pToL[3];
    float Rd[3];
    v3_from_points(pToL, point, currentLight->position);
    // calculate distance then normalize L
    float dist = v3_length(pToL);
    v3_normalize(Rd, pToL);
    v3_normalize(pToL, pToL);

    int closestObjIndex = -1;
    float tVal = shoot(&closestObjIndex, objects, Rd, point, currObjIndex);

    if (tVal >= 0 && tVal < dist) {
      // There was a valid intersection between point and light, skip over calculations for light
      continue;
    }

    Object *currObj = &objects[currObjIndex];

    // get surface normal
    float surfaceNorm[3];
    if (currObj->kind == 3) {
      v3_copy(surfaceNorm, currObj->normal);
    }
    else if (currObj->kind == 2)
    {
      v3_from_points(surfaceNorm, currObj->position, point);
      v3_normalize(surfaceNorm, surfaceNorm);
    }

    // radial attenuation
    float radAttn = 1 / (currentLight->radial_a0 + (currentLight->radial_a1 * dist) + (currentLight->radial_a2 * dist * dist));

    // angular attn
    float V0[3];
    v3_copy(V0, pToL);
    v3_scale(V0, -1);
    float angAttn = 1;
    if (currentLight->kind == 2) {
      // Vl is spot light direction
      float angAttnDot = v3_dot_product(V0, currentLight->direction);

      // check that vl is in the cone (angle < theta, dot > acos(theta))
      if (angAttnDot > currentLight->spotlightDotProd) {
        angAttn = powf(angAttnDot, currentLight->angular_a0);
      }
      else {
        angAttn = 0;
      }
    }

    // calculate diffuse component:
    // color += diffuse * attenuation (radial and angular)
    float diffuse[3] = {0, 0, 0};
    float dotProd = v3_dot_product(surfaceNorm, pToL);
    // if n dot l < 0 then diffuse is zero
    if (dotProd > 0) {
      // (n * L) (c_l) (c_m)
      diffuse[0] = dotProd * currentLight->color[0] * currObj->diffuse[0] * angAttn * radAttn;
      diffuse[1] = dotProd * currentLight->color[1] * currObj->diffuse[1] * angAttn * radAttn;
      diffuse[2] = dotProd * currentLight->color[2] * currObj->diffuse[2] * angAttn * radAttn;
    }

    // calculate specular component:
    // color += specular * attenuation (radial and angular)
    float specular[3] = {0, 0, 0};
    float R[3];
    // uL is the negative of normalized L
    float uL[3];
    v3_copy(uL, pToL);
    v3_scale(uL, -1);
    v3_reflect(R, uL, surfaceNorm);
    float viewVec[3];
    // float cam[3] = {0, 0, 0}
    v3_from_points(viewVec, point, rayInit);
    v3_normalize(viewVec, viewVec);
    // v3_normalize(R, R);
    float viewDotProd = v3_dot_product(R, viewVec);
    if (dotProd > 0 && viewDotProd > 0) {
      float shiny = powf(viewDotProd, currObj->ns);
      specular[0] = shiny * currentLight->color[0] * currObj->specular[0] * angAttn * radAttn;
      specular[1] = shiny * currentLight->color[1] * currObj->specular[1] * angAttn * radAttn;
      specular[2] = shiny * currentLight->color[2] * currObj->specular[2] * angAttn * radAttn;
    }

    v3_add(lightsColor, lightsColor, diffuse);
    v3_add(lightsColor, lightsColor, specular);
  }

  // add ambient light to color
  float ambient[3] = {0.01, 0.01, 0.01};
  v3_add(finalColor, finalColor, ambient);

  Object *surfaceObj = &objects[currObjIndex];

  float reflectAmount = 1 - surfaceObj->reflectivity;
  v3_scale(lightsColor, reflectAmount);
  v3_add(finalColor, lightsColor, finalColor);

  // if there is no reflectivity, no reason to continue calculations
  if (surfaceObj->reflectivity == 0) {
    if (finalColor[0] > 1) {
      finalColor[0] = 1;
    }
    if (finalColor[1] > 1) {
      finalColor[1] = 1;
    }
    if (finalColor[2] > 1) {
      finalColor[2] = 1;
    }

    return;
  }

  // calculate new vector - reflection ray from first ray off object with intersection
  // shoot new ray and illuminate
  // get surface normal
  float reflectSurfaceNorm[3];
  if (surfaceObj->kind == 3) {
    v3_copy(reflectSurfaceNorm, surfaceObj->normal);
  }
  else if (surfaceObj->kind == 2)
  {
    v3_from_points(reflectSurfaceNorm, surfaceObj->position, point);
    v3_normalize(reflectSurfaceNorm, reflectSurfaceNorm);
  }

  float ray[3];
  v3_from_points(ray, rayInit, point);
  v3_normalize(ray, ray);

  float reflectedRay[3];
  v3_reflect(reflectedRay, ray, reflectSurfaceNorm);

  int newClosestObjIndex = -1;
  float tVal = shoot(&newClosestObjIndex, objects, reflectedRay, point, currObjIndex);

  if (tVal > 0) {
    float reflectColor[3] = {0, 0, 0};

    float intersectPoint[3];
    v3_copy(intersectPoint, reflectedRay);
    v3_scale(intersectPoint, tVal); 
    v3_add(intersectPoint, intersectPoint, point);
    // printf("%d %d [%f %f %f] [%f %f %f] [%f %f %f]\n", currObjIndex, newClosestObjIndex, intersectPoint[0], intersectPoint[1], intersectPoint[2], reflectedRay[0], reflectedRay[1], reflectedRay[2], point[0], point[1], point[2]);
    illuminate(reflectColor, objects, lights, newClosestObjIndex, intersectPoint, point, reflectLimit);

    v3_scale(reflectColor, surfaceObj->reflectivity);
    v3_add(finalColor, reflectColor, finalColor);
  }

  if (finalColor[0] > 1) {
    finalColor[0] = 1;
  }
  if (finalColor[1] > 1) {
    finalColor[1] = 1;
  }
  if (finalColor[2] > 1) {
    finalColor[2] = 1;
  }

  // printf("%f %f %f\n", finalColor[0], finalColor[1], finalColor[2]);
}

// checks if the ray hit an object
// runs through whole list of objects checking for intersections
// returns color of closest object or black background
void intersect(float *finalColor, Object *objects, Light *lights, float *Rd, float *R0, float *cam, int *reflectLimit) {
  int closestObjIndex = -1;
  float tVal = shoot(&closestObjIndex, objects, Rd, R0, -1);

  // get color of min if there is a min
  if (tVal >= 0) {
    // There was a valid intersection, closest object is at minIndex
    // Object intersectObj = objects[minIndex];
    float intersectPoint[3];
    v3_copy(intersectPoint, Rd);
    v3_scale(intersectPoint, tVal); 
    illuminate(finalColor, objects, lights, closestObjIndex, intersectPoint, cam, reflectLimit);
  }
  else {
    finalColor[0] = 0;
    finalColor[1] = 0;
    finalColor[2] = 0;
  }
}

void read_objects(char *fileName, Object *objects, Light *lights) {
  // will eventually do the better version with dynamic array

  // Safe object storing/reading in place
  // Open file and loop through it storing values
  FILE *fh = fopen(fileName, "r");

  // while not end of file, scan in all info.
  int objIndex = 0;
  int lightIndex = 0;
  while (!feof(fh)) {
    char objCase[100];
    fscanf(fh, "%s", objCase);

    // Check if light first
    if (strcmp(objCase, "light,") == 0) {
      // One light per line
      Light light;

      // Set defaults
      light.kind = 0;
      light.position[0] = 0;
      light.position[1] = 0;
      light.position[2] = 0;
      light.color[0] = 0;
      light.color[1] = 0;
      light.color[2] = 0;
      light.theta = 0;
      // point light defaults
      light.radial_a0 = 0;
      light.radial_a1 = 0;
      light.radial_a2 = 0;
      // spot light defaults
      light.angular_a0 = 0;
      light.direction[0] = 0;
      light.direction[1] = 0;
      light.direction[2] = 0;

      char check = fgetc(fh);
      ungetc(check, fh);

      // assign the rest of the values into the light as they are seen on the row
      while (check != '\n' && check != EOF) {
        fscanf(fh, "%s", objCase);
        char v1[100], v2[100], v3[100];

        if (strcmp(objCase, "color:") == 0) {
          // need to have this longer 3 part parse because fscanf only grabs first value
          // before the space and reads it improperly
          fscanf(fh, "%s %s %s", &v1, &v2, &v3);
          sscanf(v1, "[%f,", &light.color[0]);
          sscanf(v2, "%f,", &light.color[1]);
          sscanf(v3, "%f],", &light.color[2]);
        }
        else if (strcmp(objCase, "position:") == 0) {
          fscanf(fh, "%s %s %s", &v1, &v2, &v3);
          sscanf(v1, "[%f,", &light.position[0]);
          sscanf(v2, "%f,", &light.position[1]);
          sscanf(v3, "%f],", &light.position[2]);
        }
        else if (strcmp(objCase, "direction:") == 0) {
          fscanf(fh, "%s %s %s", &v1, &v2, &v3);
          sscanf(v1, "[%f,", &light.direction[0]);
          sscanf(v2, "%f,", &light.direction[1]);
          sscanf(v3, "%f],", &light.direction[2]);

          // normalize direction vector
          v3_normalize(light.direction, light.direction);
        }
        else if (strcmp(objCase, "radial-a0:") == 0) {
          fscanf(fh, "%f", &light.radial_a0);
        }
        else if (strcmp(objCase, "radial-a1:") == 0) {
          fscanf(fh, "%f", &light.radial_a1);
        }
        else if (strcmp(objCase, "radial-a2:") == 0) {
          fscanf(fh, "%f", &light.radial_a2);
        }
        else if (strcmp(objCase, "theta:") == 0) {
          fscanf(fh, "%f", &light.theta);

          // calculate acos for future use
          float PI = 3.14159265359;
          light.spotlightDotProd = acosf((light.theta * PI) / 180);
        }
        else if (strcmp(objCase, "angular-a0:") == 0) {
          fscanf(fh, "%f", &light.angular_a0);
        }

        check = fgetc(fh);
      }

      // assign kind based on theta
      if (light.theta == 0) {
        light.kind = 1;
      }
      else {
        light.kind = 2;
      }

      // assign light then increment index
      lights[lightIndex] = light;
      lightIndex += 1;
    }

    // It's an object not a light
    else {
      // only one object per line
      Object obj;

      // defaults of 0
      obj.kind = 0;
      obj.diffuse[0] = 0;
      obj.diffuse[1] = 0;
      obj.diffuse[2] = 0;
      obj.specular[0] = 0;
      obj.specular[1] = 0;
      obj.specular[2] = 0;
      obj.position[0] = 0;
      obj.position[1] = 0;
      obj.position[2] = 0;
      obj.reflectivity - 0;
      obj.ns = 20;

      // check the case (camera, sphere, plane)
      // assign defaults in case of missing params
      if (strcmp(objCase, "camera,") == 0) {
        obj.kind = 1;

        obj.width = 0;
        obj.height = 0;
      }
      else if (strcmp(objCase, "sphere,") == 0) {
        obj.kind = 2;
        
        obj.radius = 0;
      }
      else if (strcmp(objCase, "plane,") == 0) {
        obj.kind = 3;
        
        obj.normal[0] = 0;
        obj.normal[1] = 0;
        obj.normal[2] = 0;
      }

      char check = fgetc(fh);
      ungetc(check, fh);

      // assign the rest of the values into the object as they are seen on the row
      while (check != '\n' && check != EOF) {
        fscanf(fh, "%s", objCase);
        char v1[100], v2[100], v3[100];

        if (strcmp(objCase, "diffuse_color:") == 0) {
          // need to have this longer 3 part parse because fscanf only grabs first value
          // before the space and reads it improperly
          fscanf(fh, "%s %s %s", &v1, &v2, &v3);
          sscanf(v1, "[%f,", &obj.diffuse[0]);
          sscanf(v2, "%f,", &obj.diffuse[1]);
          sscanf(v3, "%f],", &obj.diffuse[2]);
        }
        if (strcmp(objCase, "specular_color:") == 0) {
          // need to have this longer 3 part parse because fscanf only grabs first value
          // before the space and reads it improperly
          fscanf(fh, "%s %s %s", &v1, &v2, &v3);
          sscanf(v1, "[%f,", &obj.specular[0]);
          sscanf(v2, "%f,", &obj.specular[1]);
          sscanf(v3, "%f],", &obj.specular[2]);
        }
        else if (strcmp(objCase, "position:") == 0) {
          fscanf(fh, "%s %s %s", &v1, &v2, &v3);
          sscanf(v1, "[%f,", &obj.position[0]);
          sscanf(v2, "%f,", &obj.position[1]);
          sscanf(v3, "%f],", &obj.position[2]);
        }
        else if (strcmp(objCase, "normal:") == 0) {
          fscanf(fh, "%s %s %s", &v1, &v2, &v3);
          sscanf(v1, "[%f,", &obj.normal[0]);
          sscanf(v2, "%f,", &obj.normal[1]);
          sscanf(v3, "%f],", &obj.normal[2]);

          // normalize normal vector
          v3_normalize(obj.normal, obj.normal);
        }
        else if (strcmp(objCase, "width:") == 0) {
          fscanf(fh, "%f", &obj.width);
        }
        else if (strcmp(objCase, "height:") == 0) {
          fscanf(fh, "%f", &obj.height);
        }
        else if (strcmp(objCase, "radius:") == 0) {
          fscanf(fh, "%f", &obj.radius);
        }
        else if (strcmp(objCase, "reflectivity:") == 0) {
          fscanf(fh, "%f", &obj.reflectivity);
        }
        else if (strcmp(objCase, "ns:") == 0) {
          fscanf(fh, "%f", &obj.ns);
        }

        check = fgetc(fh);
      }

      // assign object then increment index
      objects[objIndex] = obj;
      objIndex += 1;
    }
  }

  // Set all leftover space to default objects
  while (objIndex < 128) {
    Object defaultObj;
    defaultObj.kind = 0;

    objects[objIndex] = defaultObj;

    objIndex += 1;
  }
  // Same for lights
  while (lightIndex < 128) {
    Light defaultLight;
    defaultLight.kind = 0;

    lights[lightIndex] = defaultLight;

    lightIndex += 1;
  }

  fclose(fh);
}

void write_P6 (char *filename, int width, int height, uint8_t *image) {
  FILE *fh = fopen(filename,"wb");
  fprintf(fh,"P6 %d %d 255\n", width, height);
  fwrite(image, sizeof(uint8_t), width*height*3, fh);
  fclose(fh);
}

void generate_image(int pixelWidth, int pixelHeight, char *fileName, char *outputFile) {
  // time measurement
  clock_t time = clock();

  // create p6 output file
  uint8_t *rgbFile = (uint8_t *) malloc(pixelWidth * pixelHeight * 3 * sizeof(uint8_t));

  // Read in the scene
  Object objects[128];
  Light lights[128];

  read_objects(fileName, objects, lights);

  // find width, height, and position from camera
  // default values of 1 if no camera provided
  float width = 1;
  float height = 1;
  float camPosition[3];
  for (int index = 0; index < 128; index += 1) {
    if (objects[index].kind == 1) {
      width = objects[index].width;
      height = objects[index].height;
      camPosition[0] = objects[index].position[0];
      camPosition[1] = objects[index].position[1];
      camPosition[2] = objects[index].position[2];
    }
  }

  // shoot ray through each pixel
  // for each ray, go through list of objects and check for intersections
  // smallest intersection (where t > 0) gets the color

  float pixel_height = height / pixelHeight;
  float pixel_width = width / pixelWidth;
  float pixelPoint[3];
  float Rd[3];

  int rgbIndex = 0;
  for (int row = 0; row < pixelHeight; row += 1) {
    pixelPoint[1] = (camPosition[1] + height) / 2 - pixel_height * (row + 0.5);
    for (int col = 0; col < pixelWidth; col += 1) {
      pixelPoint[0] = (camPosition[0] - width) / 2 + pixel_width * (col + 0.5);
      pixelPoint[2] = -1;

      v3_normalize(Rd, pixelPoint);

      // get ray and check intersections to get color
      float currColor[3] = {0, 0, 0};

      // get_Rd(ray, height, width, pixelHeight, pixelWidth, col, row);

      int reflectLimit = 5;

      intersect(currColor, objects, lights, Rd, camPosition, camPosition, &reflectLimit);
      
      // add color to uint8_t data thing (uint8_t)
      rgbFile[rgbIndex + 0] = (uint8_t)(currColor[0] * 255);
      rgbFile[rgbIndex + 1] = (uint8_t)(currColor[1] * 255);
      rgbFile[rgbIndex + 2] = (uint8_t)(currColor[2] * 255);

      rgbIndex += 3;
    }
  }

  // turn uint8_t data into image
  write_P6(outputFile, pixelWidth, pixelHeight, rgbFile);

  free(rgbFile);

  // final time measurement
  time = clock() - time;
  displayTime(time);
}
