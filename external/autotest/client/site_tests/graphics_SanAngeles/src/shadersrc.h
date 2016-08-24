// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Adapted from the javascript implementation upon WebGL by kwaters@.

#ifndef SHADERSRC_H_INCLUDED
#define SHADERSRC_H_INCLUDED

static const char *sFlatVertexSource =
    "attribute highp vec3 pos;\n"
    "attribute lowp vec4 colorIn;\n"
    "uniform highp mat4 mvp;\n"
    "varying lowp vec4 color;\n"
    "void main() {\n"
    "  color = colorIn;\n"
    "  gl_Position = mvp * vec4(pos.xyz, 1.);\n"
    "}\n";

static const char *sFlatFragmentSource =
    "varying lowp vec4 color;\n"
    "void main() {\n"
    "  gl_FragColor = vec4(color.rgb, 1.0);\n"
    "}\n";

static const char *sLitVertexSource =
    "attribute highp vec3 pos;\n"
    "attribute highp vec3 normal;\n"
    "attribute lowp vec4 colorIn;\n"
    "\n"
    "varying lowp vec4 color;\n"
    "\n"
    "uniform highp mat4 mvp;\n"
    "uniform highp mat3 normalMatrix;\n"
    "uniform lowp vec4 ambient;\n"
    "uniform lowp float shininess;\n"
    "uniform lowp vec3 light_0_direction;\n"
    "uniform lowp vec4 light_0_diffuse;\n"
    "uniform lowp vec4 light_0_specular;\n"
    "uniform lowp vec3 light_1_direction;\n"
    "uniform lowp vec4 light_1_diffuse;\n"
    "uniform lowp vec3 light_2_direction;\n"
    "uniform lowp vec4 light_2_diffuse;\n"
    "\n"
    "highp vec3 worldNormal;\n"
    "\n"
    "lowp vec4 SpecularLight(highp vec3 direction,\n"
    "                        lowp vec4 diffuseColor,\n"
    "                        lowp vec4 specularColor) {\n"
    "  lowp vec3 lightDir = normalize(direction);\n"
    "  lowp float diffuse = max(0., dot(worldNormal, lightDir));\n"
    "  lowp float specular = 0.;\n"
    "  if (diffuse > 0.) {\n"
    "    highp vec3 halfv = normalize(lightDir + vec3(0., 0., 1.));\n"
    "    specular = pow(max(0., dot(halfv, worldNormal)), shininess);\n"
    "  }\n"
    "  return diffuse * diffuseColor * colorIn + specular * specularColor;\n"
    "}\n"
    "\n"
    "lowp vec4 DiffuseLight(highp vec3 direction, lowp vec4 diffuseColor) {\n"
    "  highp vec3 lightDir = normalize(direction);\n"
    "  lowp float diffuse = max(0., dot(worldNormal, lightDir));\n"
    "  return diffuse * diffuseColor * colorIn;\n"
    "}\n"
    "\n"
    "void main() {\n"
    "  worldNormal = normalize(normalMatrix * normal);\n"
    "\n"
    "  gl_Position = mvp * vec4(pos, 1.);\n"
    "\n"
    "  color = ambient * colorIn;\n"
    "  color += SpecularLight(light_0_direction, light_0_diffuse,\n"
    "                         light_0_specular);\n"
    "  color += DiffuseLight(light_1_direction, light_1_diffuse);\n"
    "  color += DiffuseLight(light_2_direction, light_2_diffuse);\n"
    "}\n";

static const char *sFadeVertexSource =
    "attribute highp vec2 pos;\n"
    "\n"
    "varying lowp vec4 color;\n"
    "\n"
    "uniform lowp float minFade;\n"
    "\n"
    "void main() {\n"
    "  color = vec4(minFade, minFade, minFade, 1.);\n"
    "  gl_Position = vec4(pos, 0., 1.);\n"
    "}\n";

#endif  // SHADERSRC_H_INCLUDED

