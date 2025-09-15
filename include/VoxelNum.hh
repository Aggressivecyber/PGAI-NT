#ifndef VOXELNUM_HH
#define VOXELNUM_HH 1
class voxelNum
{
private:
	int voxelNx;
	int voxelNy;
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