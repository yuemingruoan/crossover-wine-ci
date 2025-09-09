/* GStreamer
 * Copyright (C) 2023 Seungha Yang <seungha@centricular.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

cbuffer SnowConstBuffer : register(b0)
{
  float time;
  float alpha;
};

struct PS_INPUT
{
  float4 Position : SV_POSITION;
  float2 Texture : TEXCOORD;
};

float get_rand (float2 uv)
{
  return frac (sin (dot (uv, float2 (12.9898,78.233))) * 43758.5453);
}

float4 PSMain_snow (PS_INPUT input) : SV_Target
{
  float4 output;
  float val = get_rand (time * input.Texture);
  output.rgb = float3(val, val, val);
  output.a = alpha;
  return output;
}

