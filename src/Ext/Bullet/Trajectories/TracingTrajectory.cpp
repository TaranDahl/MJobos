#include "TracingTrajectory.h"
#include <Ext/BulletType/Body.h>

std::unique_ptr<PhobosTrajectory> TracingTrajectoryType::CreateInstance() const
{
	return std::make_unique<TracingTrajectory>(this);
}

template<typename T>
void TracingTrajectoryType::Serialize(T& Stm)
{
	Stm
		.Process(this->TheDuration, false)
		;
}

bool TracingTrajectoryType::Load(PhobosStreamReader& Stm, bool RegisterForChange)
{
	this->PhobosTrajectoryType::Load(Stm, false);
	this->Serialize(Stm);
	return true;
}

bool TracingTrajectoryType::Save(PhobosStreamWriter& Stm) const
{
	this->PhobosTrajectoryType::Save(Stm);
	const_cast<TracingTrajectoryType*>(this)->Serialize(Stm);
	return true;
}

void TracingTrajectoryType::Read(CCINIClass* const pINI, const char* pSection)
{
	INI_EX exINI(pINI);
	this->TheDuration.Read(exINI, pSection, "Trajectory.Tracing.TheDuration");
}

template<typename T>
void TracingTrajectory::Serialize(T& Stm)
{
	Stm
		.Process(this->Type)
		.Process(this->ExistTimer)
		;
}

bool TracingTrajectory::Load(PhobosStreamReader& Stm, bool RegisterForChange)
{
	this->Serialize(Stm);
	return true;
}

bool TracingTrajectory::Save(PhobosStreamWriter& Stm) const
{
	const_cast<TracingTrajectory*>(this)->Serialize(Stm);
	return true;
}

void TracingTrajectory::OnUnlimbo(BulletClass* pBullet, CoordStruct* pCoord, BulletVelocity* pVelocity)
{
	const TracingTrajectoryType* const pType = this->Type;
	this->InitializeDuration(pBullet, pType->TheDuration);
}

bool TracingTrajectory::OnAI(BulletClass* pBullet)
{
	if (auto const pTechno = pBullet->Owner)
		pBullet->Target = pTechno->Target;

	if (auto const pTarget = pBullet->Target)
		pBullet->TargetCoords = pTarget->GetCoords();

	this->ChangeVelocity(pBullet);

	if (!this->ExistTimer.Completed())
		return false;

	return true;
}

void TracingTrajectory::OnAIPreDetonate(BulletClass* pBullet)
{
//	pBullet->UnInit(); //Prevent damage again.
}

void TracingTrajectory::OnAIVelocity(BulletClass* pBullet, BulletVelocity* pSpeed, BulletVelocity* pPosition)
{
	pSpeed->Z += BulletTypeExt::GetAdjustedGravity(pBullet->Type);
}

TrajectoryCheckReturnType TracingTrajectory::OnAITargetCoordCheck(BulletClass* pBullet)
{
	return TrajectoryCheckReturnType::SkipGameCheck;
}

TrajectoryCheckReturnType TracingTrajectory::OnAITechnoCheck(BulletClass* pBullet, TechnoClass* pTechno)
{
	return TrajectoryCheckReturnType::SkipGameCheck;
}

void TracingTrajectory::InitializeDuration(BulletClass* pBullet, int duration)
{
	if (duration <= 0)
	{
		if (const WeaponTypeClass* const pWeapon = pBullet->WeaponType)
			duration = (pWeapon->ROF > 10) ? pWeapon->ROF - 10 : 1;
		else
			duration = 120;
	}

	this->ExistTimer.Start(duration);
}

void TracingTrajectory::ChangeVelocity(BulletClass* pBullet)
{
	const TracingTrajectoryType* const pType = this->Type;
	const CoordStruct distanceCoords = pBullet->TargetCoords - pBullet->Location;
	const double distance = distanceCoords.Magnitude();

	pBullet->Velocity.X = static_cast<double>(distanceCoords.X);
	pBullet->Velocity.Y = static_cast<double>(distanceCoords.Y);
	pBullet->Velocity.Z = static_cast<double>(distanceCoords.Z);

	if (distance > pType->Trajectory_Speed)
		pBullet->Velocity *= pType->Trajectory_Speed / distance;
}
