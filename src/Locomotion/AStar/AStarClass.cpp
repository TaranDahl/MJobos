#include "AStarClass.h"

#ifdef ENABLE_ASTAR_REIMPL

#include <TubeClass.h>
#include <Ext/Techno/Body.h>

// 用外积模长衡量哪个向量与目标向量夹角更小
auto CrossProductMagnitude = [](const CellStruct& a, const CellStruct& b)
	{
		// 外积模长公式：|a.x*b.y - a.y*b.x|
		return std::abs(a.X * b.Y - a.Y * b.X);
	};

#pragma region NextPathCell

CellStruct AStarClass::NextPathCell(
	const CellStruct cell,
	const int dir
)
{
	if (dir != 8)
		return cell + Unsorted::AdjacentCell[dir];

	const int tubeIndex = MapClass::Instance.GetCellAt(cell)->TubeIndex;
	return (tubeIndex == -1) ? CellStruct::Empty : TubeClass::Array.Items[tubeIndex]->ExitCell;
}

#pragma endregion

#pragma region ProcessFinalPath

void AStarClass::ProcessFinalPath(
	PathFinderData* const pPath,
	const FootClass* const pFoot
) const
{
	// 跳过这个函数似乎对寻路效果影响不大?
	return;
	const int pathLength = pPath->PathLength - 1;

	if (pathLength <= 0)
		return;

	const int* const pLevels = pPath->Levels;
	int* const pDirs = pPath->Directions;

	// 初始化变量
	int lastDir = -1;
	int diagonalDir = -1;
	int step = 0;
	int segmentLength = 0;
	bool adjustSegment = false;
	int adjustIndex = 0;
	int adjustOffset = 0;

	// 初始化当前和下一个单元格位置
	auto currentCell = pPath->StartCell;
	auto nextCell = currentCell;

	// 遍历整个路径
	do
	{
		if (adjustSegment)
		{
			// 需要调整
			if (pDirs[adjustOffset + adjustIndex] != lastDir)
			{
				// 方向改变，完成当前段的调整
				step += AStarClass::AdjustFinalPath(pFoot, &pDirs[step], &pLevels[step], segmentLength, adjustOffset, &currentCell);

				// 重置状态，开始新段
				segmentLength = 1;
				adjustSegment = false;

				nextCell = Unsorted::AdjacentCell[pDirs[step] & 7] + currentCell;
				diagonalDir = pDirs[step];
			}
			else
			{
				// 方向未改变，继续累积调整偏移量
				++adjustOffset;
			}
		}
		else
		{
			// 正常处理
			const int currentDir = pDirs[step + segmentLength];
			const int diffDir = (currentDir - diagonalDir) & 7;

			if (currentDir == diagonalDir)
			{
				// 方向相同，增加直线段长度
				++segmentLength;
			}
			else if ((diffDir != 2 && diffDir != 6) // 不是90度转角
				|| diagonalDir == -1 // 无效的对角线方向
				|| diagonalDir == 8 // 对角线方向是隧道
				|| currentDir == 8) // 当前方向是隧道
			{
				// 不满足调整条件，直接开始新段
				step += segmentLength;
				segmentLength = 1;

				// 更新对角线方向
				diagonalDir = (currentDir & 1) ? currentDir : -1;
				currentCell = nextCell;
			}
			else
			{
				// 满足调整条件，准备进入调整
				adjustSegment = true;
				lastDir = pDirs[step + segmentLength];
				adjustOffset = 1;
				adjustIndex = step + segmentLength;
			}

			nextCell = AStarClass::NextPathCell(nextCell, currentDir);
		}
	}
	while ((step + segmentLength) < pathLength && (adjustOffset + adjustIndex) < pathLength);

	// 处理最后一个未完成的调整段
	if (adjustSegment)
		AStarClass::AdjustFinalPath(pFoot, &pDirs[step], &pLevels[step], segmentLength, adjustOffset, &currentCell);
}

#pragma endregion

#pragma region AdjustFinalPath

int AStarClass::AdjustFinalPath(
	const FootClass* const pFoot,
	int* const pDirs,
	const int* const pLevels,
	const int steps,
	int offset,
	CellStruct* const pCurrent
) const
{
	// 获取当前段的起始和结束方向
	const int firstDir = pDirs[0];
	const int lastDir = pDirs[steps];

	int avgDir = (firstDir + lastDir) >> 1;
	{
		// 检查平均方向是否有效
		const int checkDir = avgDir + 1;

		if (checkDir != lastDir && checkDir != firstDir)
			avgDir = 0;
	}

	// 隧道
	if (firstDir == 8 || lastDir == 8)
	{
		const int count = steps + offset;

		if (count > 0)
		{
			// 沿隧道方向移动整个段
			int i = 0;
			auto cell = *pCurrent;

			do
			{
				cell = AStarClass::NextPathCell(cell, pDirs[i]);
			}
			while (++i < count);

			*pCurrent = cell;
		}

		return count;
	}

	// 准备计算中间位置
	auto midCell = *pCurrent;

	// 调整偏移量确保不超过总步数
	if (steps < offset)
	{
		offset = steps;
	}
	else if (offset < steps)
	{
		const int count = steps - offset;

		if (count > 0)
		{
			// 移动到中间位置
			int i = 0;
			auto cell = midCell;

			do
			{
				cell = AStarClass::NextPathCell(cell, pDirs[i]);
			}
			while (++i < count);

			// 记下中间位置
			midCell = cell;
		}
	}

	const double threat = reinterpret_cast<double(__thiscall*)(const FootClass*)>(0x4DC760)(pFoot); // GetThreatAvoidanceCoefficient
	const auto pOwner = pFoot->Owner;

	// 路径调整
	if (offset > 0)
	{
		const auto& dirOffset = Unsorted::AdjacentCell[avgDir & 7];

		// 剩余需要处理的偏移量
		int remainingOffset = 2 * offset;

		do
		{
			int currentLevel = pLevels[steps + offset - remainingOffset];

			// 尝试移动到的单元格
			auto nextCell = midCell + dirOffset;
			auto pCell = MapClass::Instance.GetCellAt(nextCell);

			int i = remainingOffset;
			bool blocked = false;

			do
			{
				if (i <= 0)
				{
					// 处理完所有偏移量，用平均方向填充整个路径段
					const int size = 2 * offset;
					const int count = steps - offset;

					if (size > 0)
						std::fill_n(pDirs + count, size, avgDir);

					if (count > 0)
					{
						int j = 0;
						auto cell = *pCurrent;

						do
						{
							cell = AStarClass::NextPathCell(cell, pDirs[j]);
						}
						while (++j < count);

						*pCurrent = cell;
					}

					return count;
				}

				// 检查路径可行性
				blocked = (pFoot->IsCellOccupied(pCell, static_cast<FacingType>(avgDir), currentLevel, nullptr, true) != Move::OK)
					|| (pCell->Flags & CellFlags::Tube)
					|| (MapClass::Instance.GetThreatPosed(midCell, pOwner) * threat > 1.0);

				--i;

				// 继续沿平均方向移动
				nextCell += dirOffset;
				pCell = MapClass::Instance.GetCellAt(nextCell);

				// 更新高度
				const int level = pCell->Level;
				const int upLevel = level + 4;
				currentLevel = (currentLevel == upLevel && pCell->ContainsBridge()) ? upLevel : level;
			}
			while (!blocked);

			// 调整位置继续尝试
			midCell += Unsorted::AdjacentCell[firstDir & 7];
			remainingOffset -= 2;
		}
		while (--offset > 0);
	}

	if (steps > 0)
	{
		// 如果没有进行优化调整，则按原路径移动
		int i = 0;
		auto cell = *pCurrent;

		do
		{
			cell = AStarClass::NextPathCell(cell, pDirs[i]);
		}
		while (++i < steps);

		*pCurrent = cell;
	}

	return steps;
}

#pragma endregion

#pragma region OptimizeFinalPath

void AStarClass::OptimizeFinalPath(
	AStarClass::PathFinderData* const pPath,
	const FootClass* const pFoot
) const
{
	int* const pDirs = pPath->Directions;
	const int pathLength = pPath->PathLength - 1;

	if (pathLength > 0)
	{
		auto MaxCellAxisDeviation = [](const CellStruct cell) { return Math::max(std::abs(cell.X), std::abs(cell.Y)); };
		const int* const pLevels = pPath->Levels;
		const int* pCurDir = pDirs;

		// 状态初始化
		auto currentPosition = pPath->StartCell;
		auto lastValidPosition = CellStruct::Empty;
		auto totalMovementOffset = CellStruct::Empty;
		auto segmentMovementOffset = CellStruct::Empty;
		auto maxMovementDeviation = Point2D::Empty;
		int maxSegmentDiagonal = 0;
		int processedStepCount = 0;
		int lastOptimizedIndex = 0;
		int lastValidOptimizedIndex = 0;

		// 遍历整个路径
		do
		{
			// 最多优化20步
			if (processedStepCount >= 20)
				break;

			const int currentDirection = *pCurDir;

			// 隧道
			if (currentDirection == 8)
			{
				// 移动到下一单元格
				currentPosition += Unsorted::AdjacentCell[0];
				++processedStepCount;
				++pCurDir;

				// 重置所有跟踪变量并继续
				lastOptimizedIndex = processedStepCount;
				lastValidOptimizedIndex = processedStepCount;
				maxMovementDeviation = Point2D::Empty;
				maxSegmentDiagonal = 0;
				segmentMovementOffset = CellStruct::Empty;
				totalMovementOffset = CellStruct::Empty;
				lastValidPosition = CellStruct::Empty;
				continue;
			}

			if (currentDirection == -2)
			{
				// 跳过无效方向标记并继续
				++processedStepCount;
				++pCurDir;
				continue;
			}

			const auto& dirOffset = Unsorted::AdjacentCell[currentDirection & 7];
			const auto newSegmentMovementOffset = segmentMovementOffset + dirOffset;
			const auto newTotalMovementOffset = totalMovementOffset + dirOffset;
			const auto newMovementDeviation = Point2D { std::abs(newTotalMovementOffset.X), std::abs(newTotalMovementOffset.Y) };

			if (newMovementDeviation.X < maxMovementDeviation.X || newMovementDeviation.Y < maxMovementDeviation.Y)
			{
				// 发现某个方向的偏移量变小，回退状态
				if (lastValidPosition != CellStruct::Empty)
				{
					// 回退到有记录的状态
					lastOptimizedIndex = lastValidOptimizedIndex;
					segmentMovementOffset = lastValidPosition - currentPosition;
					maxSegmentDiagonal = MaxCellAxisDeviation(segmentMovementOffset);
					maxMovementDeviation = Point2D::Empty;
					totalMovementOffset = CellStruct::Empty;
					lastValidPosition = currentPosition;
					lastValidOptimizedIndex = processedStepCount;
				}
				else
				{
					// 无可回退状态，重置跟踪变量，设置当前位置为优化位置
					maxMovementDeviation = Point2D::Empty;
					totalMovementOffset = CellStruct::Empty;
					lastValidPosition = currentPosition;
					lastValidOptimizedIndex = processedStepCount;
				}
			}
			else
			{
				// 偏移量未变小，更新最大偏移量
				maxMovementDeviation = newMovementDeviation;
				totalMovementOffset = newTotalMovementOffset;

				const int newDiagonalOffset = MaxCellAxisDeviation(newSegmentMovementOffset);
				currentPosition += Unsorted::AdjacentCell[currentDirection & 7];

				if (maxSegmentDiagonal >= newDiagonalOffset)
				{
					// 对角线偏移量未增加，可以优化这段路径为直线
					int step = 0;
					auto cell = currentPosition;
					AStarClass::Instance.GetFinalStepCell(pDirs, processedStepCount, lastValidOptimizedIndex, &step, &cell);

					const auto vecCell = currentPosition - cell;
					const int plotLength = processedStepCount - step + 1;
					AStarClass::PlotStraightPath(&pDirs[step], plotLength, &cell, &vecCell, pFoot, pLevels[step], false);
				}
				else
				{
					// 对角线偏移量增加，更新最大对角线偏移量
					maxSegmentDiagonal = newDiagonalOffset;
				}

				// 记录并继续
				++processedStepCount;
				++pCurDir;
				segmentMovementOffset = newSegmentMovementOffset;
			}
		}
		while (processedStepCount < pathLength);

		// 之前存在可优化路径，最后检查剩余路径是否可以优化
		if (lastValidPosition != CellStruct::Empty)
		{
			const int finalDiagonalOffset = MaxCellAxisDeviation(currentPosition - lastValidPosition);
			const int endStep = processedStepCount - 1;

			if (endStep - lastValidOptimizedIndex > finalDiagonalOffset)
			{
				// 最后一段路没有尽量沿着斜线走，可以尝试优化
				int step = 0;
				auto cell = currentPosition;
				AStarClass::Instance.GetFinalStepCell(pDirs, endStep, lastValidOptimizedIndex, &step, &cell);

				const auto vecCell = currentPosition - cell;
				const int plotLength = endStep - step + 1;
				AStarClass::PlotStraightPath(&pDirs[step], plotLength, &cell, &vecCell, pFoot, pLevels[step], true);
			}
		}
	}

	// 压缩路径，移除无效方向标记
	int dir = *pDirs;
	int validStepCount = 0;

	if (dir != -1)
	{
		const int* pSourceDir = pDirs;
		int* pDestDirs = pDirs;
		int steps = 0;

		do
		{
			if (steps >= pathLength)
				break;

			// 保留非标记方向
			if (dir != -2)
			{
				++validStepCount;
				*pDestDirs++ = *pSourceDir;
			}

			dir = *(++pSourceDir);
			++steps;
		}
		while (dir != -1);
	}

	// 填充剩余路径
	int steps = validStepCount;
	const int totalLength = pPath->PathLength + 1;

	if (validStepCount < totalLength)
	{
		int* pDestDirs = &pDirs[validStepCount];

		do
		{
			*pDestDirs++ = -1;
			++steps;
		}
		while (steps < totalLength);
	}

	// 记录最终长度
	pPath->PathLength = validStepCount + 1;
}

#pragma endregion

#pragma region GetFinalStepCell

void AStarClass::GetFinalStepCell(
	const int* const pDirs,
	const int segmentEndIdx,
	const int segmentStartIdx,
	int* const pOutIdx,
	CellStruct* const pAdjacent
) const
{
	auto finalCell = *pAdjacent;
	int currentIndex = segmentStartIdx;

	if (segmentEndIdx >= segmentStartIdx)
	{
		auto accumulated = CellStruct::Empty;
		bool foundBetterPath = false;
		int maxDeviation = 0;

		// 反向遍历路径
		for (int searchIndex = segmentEndIdx; searchIndex >= segmentStartIdx; --searchIndex)
		{
			const int curDir = pDirs[searchIndex];

			// 跳过无效方向
			if (curDir != -2)
			{
				const int oppDir = (curDir - 4) & 7;
				const auto& dirOffset = Unsorted::AdjacentCell[oppDir];

				// 计算新的累积偏移量和单元格位置以及当前路径点的最大偏差
				const auto newAccumulated = accumulated + dirOffset;
				const auto newCell = finalCell + dirOffset;
				const int currentDeviation = Math::max(std::abs(newAccumulated.X), std::abs(newAccumulated.Y));

				if (currentDeviation <= maxDeviation)
				{
					// 当前偏差小于等于最大偏差，标记找到更好路径
					foundBetterPath = true;
				}
				else if (!foundBetterPath)
				{
					// 尚未找到更好路径且当前偏差更大，更新最大偏差
					maxDeviation = currentDeviation;
				}
				else
				{
					// 找到更好的路径，计算最终方向
					*pOutIdx = searchIndex + 1;
					// 因为有特殊值存在，所以保留两次反向的逻辑
					*pAdjacent = newCell + Unsorted::AdjacentCell[(oppDir - 4) & 7];
					return;
				}

				// 更新累积偏移量和当前状态
				accumulated = newAccumulated;
				currentIndex = segmentStartIdx;
				finalCell = newCell;
			}
		}
	}

	// 没有找到更好的路径，返回默认值
	*pOutIdx = currentIndex;
	*pAdjacent = finalCell;
}

#pragma endregion

#pragma region PlotStraightPath

// 尝试1：重写PlotStraightPath这个函数
// 结果：有点用但不多，这个函数经常不会被调用。
// 原本情况下调用时的pVector总是|X|==|Y|的，实际上可以说没有任何效果。
// 将ProcessFinalPath整个跳过, 则这个函数偶尔会得到|X|!=|Y|的输入，这种时候确实能够优化。
bool AStarClass::PlotStraightPath(
	int* const pDirs,
	const int maxLength,
	const CellStruct* const pCurrent,
	const CellStruct* const pVector,
	const FootClass* const pFoot,
	const int curLevel,
	const bool allowThreats
) const
{
	// 确定主要和次要移动方向
	const int vecX = pVector->X;
	const int vecY = pVector->Y;
	const int sumXY = vecY + vecX;
	int primaryDir = (vecX >= 0) ? (vecY >= 0 ? 3 : 1) : (vecY >= 0 ? 5 : 7);
	int secondaryDir = (vecX - vecY <= 0) ? (sumXY <= 0 ? 6 : 4) : (sumXY <= 0 ? 0 : 2);
	int totalSteps = Math::max(vecX, vecY);
	auto originalVec = *pVector;

	// 计算各方向步数
	const int absX = std::abs(vecX);
	const int absY = std::abs(vecY);

	// 1. 确定主要方向和次要方向
	int minSteps = Math::min(absX, absY);
	int diagSteps = Math::max(absX, absY) - minSteps;

	if (totalSteps <= 0 || totalSteps > maxLength)
		return false;


	// 2. 计算外积模长（核心：评估两个向量的夹角）
	/*auto CrossProductMagnitude = [](const CellStruct& a, const CellStruct& b)
		{
			// 外积模长公式：|a.x*b.y - a.y*b.x|
			return std::abs(a.X * b.Y - a.Y * b.X);
		};*/

	// 3. 初始化状态变量
	auto currentPos = *pCurrent;
	int currentLevel = curLevel;
	int threatCount = 0;
	int majorUsed = 0;  // 已使用的主方向步数
	int minorUsed = 0;  // 已使用的次方向步数
	const double threat = reinterpret_cast<double(__thiscall*)(const FootClass*)>(0x4DC760)(pFoot);
	const auto pOwner = pFoot->Owner;

	//Debug::LogAndMessage("Start (%d, %d), Vec (%d, %d)\n", currentPos.X, currentPos.Y, originalVec.X, originalVec.Y);

	// 4. 逐步生成路径（每步在主/次方向中选择）
	for (int step = 0; step < totalSteps; ++step)
	{
		// 检查剩余可用步数
		const bool canUseMajor = (majorUsed < minSteps);
		const bool canUseMinor = (minorUsed < diagSteps);

		if (!canUseMajor && !canUseMinor)
			break;

		// 当前位置到目标的向量
		const CellStruct currentToTarget = originalVec - (currentPos - *pCurrent);

		// 5. 计算两个方向的外积模长
		int majorCross = INT_MAX;  // 主方向的外积模长
		int minorCross = INT_MAX;  // 次方向的外积模长

		if (canUseMajor)
		{
			// 主方向移动后的新向量
			const CellStruct majorStep = Unsorted::AdjacentCell[primaryDir & 7];
			const CellStruct newToTargetMajor = currentToTarget - majorStep;
			majorCross = CrossProductMagnitude(newToTargetMajor, originalVec);
		}

		if (canUseMinor)
		{
			// 次方向移动后的新向量
			const CellStruct minorStep = Unsorted::AdjacentCell[secondaryDir & 7];
			const CellStruct newToTargetMinor = currentToTarget - minorStep;
			minorCross = CrossProductMagnitude(newToTargetMinor, originalVec);
		}

		//Debug::LogAndMessage("Step %d, majorCross %d, minorCross %d\n", majorCross, minorCross);
		// 6. 选择外积模长更小的方向（夹角更小）
		int preferredDir = -1;
		bool isPreferredMajor = false;

		if (canUseMajor && canUseMinor)
		{
			// 两者都可用：选外积模长小的
			preferredDir = (majorCross <= minorCross) ? primaryDir : secondaryDir;
			isPreferredMajor = (preferredDir == primaryDir);
		}
		else if (canUseMajor)
		{
			preferredDir = primaryDir;
			isPreferredMajor = true;
		}
		else
		{
			preferredDir = secondaryDir;
			isPreferredMajor = false;
		}

		// 7. 验证方向可行性（障碍物/威胁检查）
		auto TryMove = [&](int dir, bool isMajor) -> bool
			{
				const CellStruct newPos = currentPos + Unsorted::AdjacentCell[dir & 7];
				const auto pCell = MapClass::Instance.GetCellAt(newPos);

				// 障碍物检查
				const bool blocked = (pFoot->IsCellOccupied(pCell, static_cast<FacingType>(dir), currentLevel, nullptr, true) != Move::OK)
					|| (pCell->Flags & CellFlags::Tube);

				// 威胁检查
				int newThreat = threatCount;
				if (threat > 0.00001 && MapClass::Instance.GetThreatPosed(newPos, pOwner) * threat >= 0.01)
					newThreat++;

				const bool threatBlocked = (newThreat > 3) || (!allowThreats && newThreat > 0);

				if (!blocked && !threatBlocked)
				{
					// 移动有效：更新状态
					pDirs[step] = dir;
					currentPos = newPos;
					threatCount = newThreat;
					// 更新高度（处理桥梁）
					const int cellLevel = pCell->Level;
					const int bridgeLevel = cellLevel + 4;
					currentLevel = (currentLevel == bridgeLevel && pCell->ContainsBridge()) ? bridgeLevel : cellLevel;
					// 更新步数计数器
					isMajor ? majorUsed++ : minorUsed++;
					return true;
				}
				return false;
			};

		// 尝试首选方向
		if (TryMove(preferredDir, isPreferredMajor))
			continue;

		// 首选方向不可行，尝试另一个方向
		if (isPreferredMajor && canUseMinor)
		{
			if (TryMove(secondaryDir, false))
				continue;
		}
		else if (!isPreferredMajor && canUseMajor)
		{
			if (TryMove(primaryDir, true))
				continue;
		}

		// 两个方向都不可行
		std::fill_n(pDirs, maxLength, -2);
		return false;
	}

	// 标记剩余无效步数
	const int remainingSteps = maxLength - totalSteps;
	if (remainingSteps > 0)
		std::fill_n(pDirs + totalSteps, remainingSteps, -2);

	return true;
}

#pragma endregion

#pragma region Hooks

DEFINE_FUNCTION_JUMP(CALL, 0x42A415, AStarClass::ProcessFinalPath);
DEFINE_FUNCTION_JUMP(CALL, 0x42A41E, AStarClass::OptimizeFinalPath);

namespace FindRegularPathContext
{
	CellStruct startCell;
	int CheckNextDirAddress = 0x42A1A1;
}

// 记录寻路起始位置。
DEFINE_HOOK(0x429A9F, PathFinder_FindRegularPath_SetContext, 0x7)
{
	GET_STACK(CellStruct*, pStartMapCrd, STACK_OFFSET(0x5C, 0x4));
	FindRegularPathContext::startCell = *pStartMapCrd;
	return 0;
}

// 到达这里才能创建PathNode。
DEFINE_HOOK(0x42A022, PathFinder_FindRegularPath_CreatePathNode, 0x8)
{
	GET(float, cost, ECX);
	GET_STACK(CellClass**, pFromCellPtr, STACK_OFFSET(0x5C, -0x3C));
	GET_STACK(CellClass**, pToCellPtr, STACK_OFFSET(0x5C, -0x38));
	GET_STACK(CellStruct*, pEndMapCrd, STACK_OFFSET(0x5C, 0x8));
	//GET_STACK(CellStruct*, pStartMapCrd, STACK_OFFSET(0x5C, 0x4));
	auto pStartMapCrd = &FindRegularPathContext::startCell;

	auto vec1 = (*pFromCellPtr)->MapCoords - *pEndMapCrd;
	auto vec2 = (*pToCellPtr)->MapCoords - *pEndMapCrd;
	auto projMag = std::abs(vec1.X * vec2.X + vec1.Y * vec2.Y) / (float)(vec1.X * vec1.X + vec1.Y * vec1.Y);
	//Debug::LogAndMessage("FromCell (%d, %d), ToCell (%d, %d), EndCell(%d, %d), cross %d\n", (*pFromCellPtr)->MapCoords.X, (*pFromCellPtr)->MapCoords.Y, (*pToCellPtr)->MapCoords.X, (*pToCellPtr)->MapCoords.Y, pEndMapCrd->X, pEndMapCrd->Y, projMag);
	auto startToCurrent = (*pFromCellPtr)->MapCoords - *pStartMapCrd;
	auto currentToEnd = *pEndMapCrd - (*pFromCellPtr)->MapCoords;
	auto startToEnd = *pEndMapCrd - *pStartMapCrd;
	auto currentToNext = (*pToCellPtr)->MapCoords - (*pFromCellPtr)->MapCoords;
	//Debug::LogAndMessage("StartToEnd (%d, %d), StartToCurrent (%d, %d), CurrentToNext (%d, %d), CurrentToEnd (%d, %d)\n", startToEnd.X, startToEnd.Y, startToCurrent.X, startToCurrent.Y, currentToNext.X, currentToNext.Y, currentToEnd.X, currentToEnd.Y);
	R->ECX(projMag);
	return 0;
}

// 此处也是个什么条件，满足之后会跳过一个方向。但似乎平地寻路根本用不到这个。
DEFINE_HOOK(0x429F1B, PathFinder_FindRegularPath_CheckToSkipDir, 0x6)
{
	GET(AStarClass*, pThis, ESI);
	GET(AStarClass::PathQueueNode*, pEndNode, EDX);
	GET(int, someIdx, EDI);

	GET_STACK(CellClass**, pFromCellPtr, STACK_OFFSET(0x5C, -0x3C));
	GET_STACK(CellClass**, pToCellPtr, STACK_OFFSET(0x5C, -0x38));
	GET_STACK(CellStruct*, pEndMapCrd, STACK_OFFSET(0x5C, 0x8));
	//GET_STACK(CellStruct*, pStartMapCrd, STACK_OFFSET(0x5C, 0x4));
	auto pStartMapCrd = &FindRegularPathContext::startCell;

	auto left = pThis->Distances[someIdx];
	auto right = pEndNode->PathCost + 1.009;

	Debug::LogAndMessage("Should ? %d, left %.3f, right %.3f\n", left < right, left, right);

	return left < right ? FindRegularPathContext::CheckNextDirAddress : 0x429F37;
}

// useHierarchical 时会跳过一个方向。暂时禁用这个。
DEFINE_HOOK(0x429EC1, PathFinder_FindRegularPath_UseUnHierarchical, 0x6)
{
	GET_STACK(bool, hier, STACK_OFFSET(0x5C, 0x18));
	return 0x429EC7;
}

// 这里也会跳过一个方向，而且这里经常会走。
DEFINE_HOOK(0x429E21, PathFinder_FindRegularPath_IsNextExist, 0x6)
{
	GET(CellClass*, pNextCell, EBX);
	if (!pNextCell)
		Debug::LogAndMessage("Here1\n");
	return pNextCell ? 0x429E27 : FindRegularPathContext::CheckNextDirAddress;
}

#pragma endregion

#endif
