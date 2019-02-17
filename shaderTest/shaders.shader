struct VOut
{
	float4 position : SV_POSITION;
	float4 color : COLOR;
};

VOut VShader(
	float3 position : POSITION,
	float3 normal : NORMAL,
	float4 tangent : TANGENT,
	float2 uvp:UV_Primary,
	float2 upv2:UV_Secondary,
	float4 color:COLOR,
	float3 posPF:PositionPF,
	uint iId:SV_InstanceID
)
{
	VOut output;

	output.color = color;
	output.position.x = position.x;
	output.position.y = position.y;
	output.position.z = position.z;
	output.position.w = 1.0;

	return output;
}

float4 PShader(float4 position : SV_POSITION) : SV_TARGET
{
	return position;
}
/*float4 PShader(float4 position : SV_POSITION, float4 color : COLOR) : SV_TARGET
{
	return color;
}*/