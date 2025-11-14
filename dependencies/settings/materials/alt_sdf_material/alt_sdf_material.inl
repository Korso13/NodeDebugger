
namespace materials::alt_sdf_material
{
	inline static const char* m_vertexShader = R"(
attribute vec2 w4_a_uv0;

uniform vec2 texsize;

varying vec2 vUv;

void w4_main() {
    vUv = w4_a_uv0 / texsize;
    gl_Position = w4_u_projectionView * w4_getVertexPosition();
}
 )";

	inline static const char* m_fragmentShader = R"(
uniform sampler2D texture0;
uniform vec4 color;
uniform vec4 outlineColor;
uniform float edgeCenter;
uniform float smoothing;
uniform float outlineBorder;
uniform int useOriginalColor;
uniform float originalColorDiscardThreshold;

// uniform float outlineMin0;
// uniform float outlineMin1;
// uniform float outlineMax0;
// uniform float outlineMax1;

//#define originalColorDiscardAvg
#define originalColorDiscardLimit

#define outlineMin0 0.2
#define outlineMin1 0.3
#define outlineMax1 0.4
#define outlineMax0 0.5

varying vec2 vUv;

// https://steamcdn-a.akamaihd.net/apps/valve/2007/SIGGRAPH2007_AlphaTestedMagnification.pdf

void w4_main()
{
    if(useOriginalColor > 0)
    {
        vec4 originalColor = texture2D(texture0, vUv);

#ifdef originalColorDiscardAvg
        vec4 fullColor = vec4(1.0, 1.0, 1.0, 0.0);
        fullColor -= originalColor;
        float transparency = max(0.0, (1.0 - (fullColor.x / 1.0 +  fullColor.y / 1.0 + fullColor.z / 1.0) / 3.0));
        originalColor.a = max(transparency, originalColorDiscardThreshold);
#endif
#ifdef originalColorDiscardLimit
        if(originalColor.r <= originalColorDiscardThreshold &&  originalColor.g <= originalColorDiscardThreshold && originalColor.b <= originalColorDiscardThreshold)
        {
            originalColor.a = 0.0;
            discard;
        }
#endif
        gl_FragColor = originalColor;
        return;
    }

    float dist = texture2D(texture0, vUv).r;

    // if (dist >= outlineMin0 && dist <= outlineMax1)
    // {
    //     float factor = 1.0;
    //     if (dist <= outlineMin1)
    //     {
    //         factor = smoothstep(outlineMin0, outlineMin1, dist);
    //     }
    //     else
    //     {
    //         factor = smoothstep(outlineMax1, outlineMax0, dist);
    //     }
    //     baseColor = lerp(color, outlineColor, factor);
    // }
    //
    // if (false)
    // {
    //     float softEdgeMin = 0.1
    //     float softEdgeMax = 0.8
    //     dist *= smoothstep(softEdgeMin, softEdgeMax, dist);
    // }

    float alpha = smoothstep(edgeCenter - smoothing, edgeCenter + smoothing, dist);
    vec3 finalColor;
    if (alpha < outlineBorder) // TODO
    {
        finalColor = outlineColor.rgb;
    }
    else
    {
        finalColor = color.rgb;
    }

    // gl_FragColor = vec4(finalColor, 1.0);
    gl_FragColor = vec4(finalColor, alpha * color.a);
}
)";

}