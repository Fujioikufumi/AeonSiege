#include "SkeletonDebugDraw.h"

namespace
{
	/// <summary>
	/// �ϊ��s�񂩂�ʒu�����𒊏o����
	/// </summary>
	XMFLOAT3 ExtractPosition(const XMMATRIX& m)
	{
		return XMFLOAT3(
			m.r[3].m128_f32[0],
			m.r[3].m128_f32[1],
			m.r[3].m128_f32[2]
		);
	}
}

//void ComputeSkeletonGlobalBindMatrices(const Skeleton& skeleton, std::vector<DirectX::XMMATRIX>& outGlobalMatrices)
//{
//	const int boneCount = (int)skeleton.bones.size();
//	outGlobalMatrices.resize(boneCount);
//
//	for (int i = 0; i < boneCount; ++i)
//	{
//		const Bone& bone = skeleton.bones[i];
//
//		if (bone.parentIndex < 0)
//		{
//			outGlobalMatrices[i] = bone.localBind;
//		}
//		else
//		{
//			outGlobalMatrices[i] =
//				bone.localBind *
//				outGlobalMatrices[bone.parentIndex];
//		}
//	}
//}
//
//void BuildSkeletonDebugLines(const Skeleton& skeleton, const std::vector<DirectX::XMMATRIX>& globalMatrices, std::vector<LineVertex>& outVertices)
//{
//	outVertices.clear();
//
//	for (int i = 0; i < (int)skeleton.bones.size(); ++i)
//	{
//		const Bone& bone = skeleton.bones[i];
//
//		if (bone.parentIndex < 0)
//			continue;
//
//		XMFLOAT3 p0 = ExtractPosition(globalMatrices[bone.parentIndex]);
//		XMFLOAT3 p1 = ExtractPosition(globalMatrices[i]);
//
//		LineVertex v0{ p0, XMFLOAT4(1, 0, 0, 1) };
//		LineVertex v1{ p1, XMFLOAT4(1, 0, 0, 1) };
//
//		outVertices.push_back(v0);
//		outVertices.push_back(v1);
//	}
//}
