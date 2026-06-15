#ifndef VOXELNUM_HH
#define VOXELNUM_HH 1

// 体素矩阵维度 (与 DetectorConstruction 中 setNumX/Y 保持一致)
class voxelNum
{
private:
	int voxelNx{ 128 };
	int voxelNy{ 128 };
public:
	inline int GetNumY(int copyNo)
	{
		return copyNo % voxelNy;
	}
	inline int GetNumX(int copyNo)
	{
		return static_cast<int>(copyNo / voxelNy);
	}
	inline void setNumY(int y)
	{
		voxelNy = y;
	}
	inline void setNumX(int x)
	{
		voxelNx = x;
	}
	inline int GetNy()
	{
		return voxelNy;
	}
	inline int GetNx()
	{
		return voxelNx;
	}
};

#endif
