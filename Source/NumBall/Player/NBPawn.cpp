#include "Player/NBPawn.h"

#include "NumBall.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NBPawn)

void ANBPawn::BeginPlay()
{
	Super::BeginPlay();

	FString NetModeString = NumBallFunctionLibrary::GetRoleString(this);
	FString CombinedString = FString::Printf(TEXT("NBPawn::BeginPlay() %s [%s]")
												    , *NumBallFunctionLibrary::GetNetModeString(this)
												    , *NetModeString);
	NumBallFunctionLibrary::MyPrintString(this, CombinedString, 10.f);
}

void ANBPawn::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	FString NetModeString = NumBallFunctionLibrary::GetRoleString(this);
	FString CombinedString = FString::Printf(TEXT("NBPawn::PossessedBy() %s [%s]")
												    , *NumBallFunctionLibrary::GetNetModeString(this)
												    , *NetModeString);
	NumBallFunctionLibrary::MyPrintString(this, CombinedString, 10.f);
}
