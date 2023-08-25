RWBuffer<float> Data : register(u0);

[shader("compute")]
[numthreads(1, 1, 1)]
void csmain(uint3 DTid : SV_DispatchThreadID)
{
	Data[DTid.x] = 0.0f;
}
