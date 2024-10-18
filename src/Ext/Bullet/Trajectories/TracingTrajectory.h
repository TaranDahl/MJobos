#pragma once

#include "PhobosTrajectory.h"

class TracingTrajectoryType final : public PhobosTrajectoryType
{
public:
	TracingTrajectoryType() : PhobosTrajectoryType()
		, TheDuration { 0 }
	{ }

	virtual bool Load(PhobosStreamReader& Stm, bool RegisterForChange) override;
	virtual bool Save(PhobosStreamWriter& Stm) const override;
	virtual std::unique_ptr<PhobosTrajectory> CreateInstance() const override;
	virtual void Read(CCINIClass* const pINI, const char* pSection) override;
	virtual TrajectoryFlag Flag() const override { return TrajectoryFlag::Tracing; }

	Valueable<int> TheDuration;

private:
	template <typename T>
	void Serialize(T& Stm);
};

class TracingTrajectory final : public PhobosTrajectory
{
public:
	TracingTrajectory(noinit_t) { }

	TracingTrajectory(TracingTrajectoryType const* trajType) : Type { trajType }
		, ExistTimer {}
	{ }

	virtual bool Load(PhobosStreamReader& Stm, bool RegisterForChange) override;
	virtual bool Save(PhobosStreamWriter& Stm) const override;
	virtual TrajectoryFlag Flag() const override { return TrajectoryFlag::Tracing; }
	virtual void OnUnlimbo(BulletClass* pBullet, CoordStruct* pCoord, BulletVelocity* pVelocity) override;
	virtual bool OnAI(BulletClass* pBullet) override;
	virtual void OnAIPreDetonate(BulletClass* pBullet) override;
	virtual void OnAIVelocity(BulletClass* pBullet, BulletVelocity* pSpeed, BulletVelocity* pPosition) override;
	virtual TrajectoryCheckReturnType OnAITargetCoordCheck(BulletClass* pBullet) override;
	virtual TrajectoryCheckReturnType OnAITechnoCheck(BulletClass* pBullet, TechnoClass* pTechno) override;

	const TracingTrajectoryType* Type;
	CDTimerClass ExistTimer;

private:
	template <typename T>
	void Serialize(T& Stm);

	void InitializeDuration(BulletClass* pBullet, int duration);
	void ChangeVelocity(BulletClass* pBullet);
};
