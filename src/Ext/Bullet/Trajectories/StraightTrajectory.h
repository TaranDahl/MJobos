#pragma once

#include "PhobosTrajectory.h"

class StraightTrajectoryType final : public PhobosTrajectoryType
{
public:
	StraightTrajectoryType() : PhobosTrajectoryType(TrajectoryFlag::Straight)
		, DetonationDistance { Leptons(102) }
		, TargetSnapDistance { Leptons(128) }
		, PassThrough { false }
		, PassDetonate { false }
		, PassDetonateDelay { 1 }
		, PassDetonateTimer { 0 }
		, PassDetonateLocal { false }
		, LeadTimeCalculate { false }
		, OffsetCoord { { 0, 0, 0 } }
		, RotateCoord { 0 }
		, MirrorCoord { true }
		, UseDisperseBurst { false }
		, AxisOfRotation { { 0, 0, 1 } }
		, ProximityImpact { 0 }
		, ProximityRadius { 0.7 }
		, ProximityAllies { 0.0 }
		, ThroughVehicles { true }
		, ThroughBuilding { true }
		, StraightWarhead {}
		, StraightDamage { 0 }
		, SubjectToGround { false }
		, ConfineAtHeight { 0 }
		, EdgeAttenuation { 1.0 }
	{}

	virtual bool Load(PhobosStreamReader& Stm, bool RegisterForChange) override;
	virtual bool Save(PhobosStreamWriter& Stm) const override;

	virtual void Read(CCINIClass* const pINI, const char* pSection) override;

	Valueable<Leptons> DetonationDistance;
	Valueable<Leptons> TargetSnapDistance;
	Valueable<bool> PassThrough;
	Valueable<bool> PassDetonate;
	Valueable<int> PassDetonateDelay;
	Valueable<int> PassDetonateTimer;
	Valueable<bool> PassDetonateLocal;
	Valueable<bool> LeadTimeCalculate;
	Valueable<CoordStruct> OffsetCoord;
	Valueable<double> RotateCoord;
	Valueable<bool> MirrorCoord;
	Valueable<bool> UseDisperseBurst;
	Valueable<CoordStruct> AxisOfRotation;
	Valueable<int> ProximityImpact;
	Valueable<double> ProximityRadius;
	Valueable<double> ProximityAllies;
	Valueable<bool> ThroughVehicles;
	Valueable<bool> ThroughBuilding;
	Nullable<WarheadTypeClass*> StraightWarhead;
	Valueable<int> StraightDamage;
	Valueable<bool> SubjectToGround;
	Valueable<int> ConfineAtHeight;
	Valueable<double> EdgeAttenuation;
};

class StraightTrajectory final : public PhobosTrajectory
{
public:
	StraightTrajectory() : PhobosTrajectory(TrajectoryFlag::Straight)
		, DetonationDistance { Leptons(102) }
		, TargetSnapDistance { Leptons(128) }
		, PassThrough { false }
		, PassDetonate { false }
		, PassDetonateDelay { 1 }
		, PassDetonateTimer { 0 }
		, PassDetonateLocal { false }
		, LeadTimeCalculate { false }
		, OffsetCoord {}
		, RotateCoord { 0 }
		, MirrorCoord { true }
		, UseDisperseBurst { false }
		, AxisOfRotation {}
		, ProximityImpact { 0 }
		, ProximityRadius { 0.7 }
		, ProximityAllies { 0.0 }
		, ThroughVehicles { true }
		, ThroughBuilding { true }
		, StraightWarhead {}
		, StraightDamage { 0 }
		, SubjectToGround { false }
		, ConfineAtHeight { 0 }
		, EdgeAttenuation { 1.0 }
		, CheckTimesLimit { 0 }
		, ExtraCheck1 { nullptr }
		, ExtraCheck2 { nullptr }
		, ExtraCheck3 { nullptr }
		, LastCasualty {}
		, FirepowerMult { 1.0 }
		, LastTargetCoord {}
		, CurrentBurst { 0 }
		, CountOfBurst { 0 }
		, WaitOneFrame { 0 }
	{}

	StraightTrajectory(PhobosTrajectoryType* pType) : PhobosTrajectory(TrajectoryFlag::Straight)
		, DetonationDistance { Leptons(102) }
		, TargetSnapDistance { Leptons(128) }
		, PassThrough { false }
		, PassDetonate { false }
		, PassDetonateDelay { 1 }
		, PassDetonateTimer { 0 }
		, PassDetonateLocal { false }
		, LeadTimeCalculate { false }
		, OffsetCoord {}
		, RotateCoord { 0 }
		, MirrorCoord { true }
		, UseDisperseBurst { false }
		, AxisOfRotation {}
		, ProximityImpact { 0 }
		, ProximityRadius { 0.7 }
		, ProximityAllies { 0.0 }
		, ThroughVehicles { true }
		, ThroughBuilding { true }
		, StraightWarhead {}
		, StraightDamage { 0 }
		, SubjectToGround { false }
		, ConfineAtHeight { 0 }
		, EdgeAttenuation { 1.0 }
		, CheckTimesLimit { 0 }
		, ExtraCheck1 { nullptr }
		, ExtraCheck2 { nullptr }
		, ExtraCheck3 { nullptr }
		, LastCasualty {}
		, FirepowerMult { 1.0 }
		, LastTargetCoord {}
		, CurrentBurst { 0 }
		, CountOfBurst { 0 }
		, WaitOneFrame { 0 }
	{}

	virtual bool Load(PhobosStreamReader& Stm, bool RegisterForChange) override;
	virtual bool Save(PhobosStreamWriter& Stm) const override;

	virtual void OnUnlimbo(BulletClass* pBullet, CoordStruct* pCoord, BulletVelocity* pVelocity) override;
	virtual bool OnAI(BulletClass* pBullet) override;
	virtual void OnAIPreDetonate(BulletClass* pBullet) override;
	virtual void OnAIVelocity(BulletClass* pBullet, BulletVelocity* pSpeed, BulletVelocity* pPosition) override;
	virtual TrajectoryCheckReturnType OnAITargetCoordCheck(BulletClass* pBullet) override;
	virtual TrajectoryCheckReturnType OnAITechnoCheck(BulletClass* pBullet, TechnoClass* pTechno) override;

	Leptons DetonationDistance;
	Leptons TargetSnapDistance;
	bool PassThrough;
	bool PassDetonate;
	int PassDetonateDelay;
	int PassDetonateTimer;
	bool PassDetonateLocal;
	bool LeadTimeCalculate;
	CoordStruct OffsetCoord;
	double RotateCoord;
	bool MirrorCoord;
	bool UseDisperseBurst;
	CoordStruct AxisOfRotation;
	int ProximityImpact;
	double ProximityRadius;
	double ProximityAllies;
	bool ThroughVehicles;
	bool ThroughBuilding;
	WarheadTypeClass* StraightWarhead;
	int StraightDamage;
	bool SubjectToGround;
	int ConfineAtHeight;
	double EdgeAttenuation;
	int CheckTimesLimit;
	TechnoClass* ExtraCheck1;
	TechnoClass* ExtraCheck2;
	TechnoClass* ExtraCheck3;
	std::vector<TechnoClass*> LastCasualty;
	double FirepowerMult;
	CoordStruct LastTargetCoord;
	int CurrentBurst;
	int CountOfBurst;
	int WaitOneFrame;

private:
	void PrepareForOpenFire(BulletClass* pBullet);
	int GetVelocityZ(BulletClass* pBullet);
	bool CalculateBulletVelocity(BulletClass* pBullet, double StraightSpeed);
	bool BulletDetonatePreCheck(BulletClass* pBullet, HouseClass* pOwner);
	void BulletDetonateLastCheck(BulletClass* pBullet, HouseClass* pOwner, double StraightSpeed);
	void PassWithDetonateAt(BulletClass* pBullet, HouseClass* pOwner);
	void PrepareForDetonateAt(BulletClass* pBullet, HouseClass* pOwner);
	std::vector<CellClass*> GetCellsInPassThrough(BulletClass* pBullet);
	std::vector<CellClass*> GetCellsInProximityRadius(BulletClass* pBullet);
	std::vector<CellStruct> GetCellsInRectangle(CellStruct bStaCell, CellStruct lMidCell, CellStruct rMidCell, CellStruct tEndCell);
	double GetExtraDamageMultiplier(BulletClass* pBullet, TechnoClass* pTechno, HouseClass* pOwner, bool Self);
	bool PassAndConfineAtHeight(BulletClass* pBullet, double StraightSpeed);
};
