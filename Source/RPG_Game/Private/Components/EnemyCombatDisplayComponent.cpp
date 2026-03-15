#include "Components/EnemyCombatDisplayComponent.h"

#include "Components/PlayerStatsComponent.h"
#include "Components/TargetLockComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UI/DamageNumberWidget.h"
#include "UI/EnemyStatusBarWidget.h"

UEnemyCombatDisplayComponent::UEnemyCombatDisplayComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UEnemyCombatDisplayComponent::BeginPlay()
{
    Super::BeginPlay();

    CachedCharacter = Cast<ACharacter>(GetOwner());
    CachedStats = GetOwner() ? GetOwner()->FindComponentByClass<UPlayerStatsComponent>() : nullptr;

    EnsureStatusWidget();

    if (CachedStats.IsValid())
    {
        CachedStats->OnHealthChanged.AddDynamic(this, &UEnemyCombatDisplayComponent::HandleHealthChanged);
        CachedStats->OnHitReceived.AddDynamic(this, &UEnemyCombatDisplayComponent::HandleHitReceived);
        CachedStats->OnDeath.AddDynamic(this, &UEnemyCombatDisplayComponent::HandleDeath);
        HandleHealthChanged(CachedStats->CurrentHealth, CachedStats->MaxHealth);
    }

    UpdateFocusedState();
}

void UEnemyCombatDisplayComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    UpdateFocusedState();
}

void UEnemyCombatDisplayComponent::HandleHealthChanged(float CurrentHealth, float MaxHealth)
{
    EnsureStatusWidget();

    if (!StatusWidgetComponent)
    {
        return;
    }

    if (UEnemyStatusBarWidget* StatusWidget = Cast<UEnemyStatusBarWidget>(StatusWidgetComponent->GetUserWidgetObject()))
    {
        StatusWidget->SetHealthValues(CurrentHealth, MaxHealth);
    }
}

void UEnemyCombatDisplayComponent::HandleHitReceived(FRPGDamageSpec DamageSpec, float DamageApplied, float NewHealth, float MaxHealth)
{
    LastHitDisplayTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
    HandleHealthChanged(NewHealth, MaxHealth);
    SpawnDamageNumber(DamageApplied, DamageSpec.HitLocation);
}

void UEnemyCombatDisplayComponent::HandleDeath()
{
    if (StatusWidgetComponent)
    {
        StatusWidgetComponent->SetHiddenInGame(true);
    }
}

void UEnemyCombatDisplayComponent::EnsureStatusWidget()
{
    if (StatusWidgetComponent || !CachedCharacter.IsValid())
    {
        return;
    }

    USceneComponent* AttachParent = CachedCharacter->GetMesh() ? static_cast<USceneComponent*>(CachedCharacter->GetMesh()) : CachedCharacter->GetRootComponent();
    if (!AttachParent)
    {
        return;
    }

    StatusWidgetComponent = NewObject<UWidgetComponent>(GetOwner(), TEXT("EnemyStatusWidget"));
    if (!StatusWidgetComponent)
    {
        return;
    }

    StatusWidgetComponent->SetupAttachment(AttachParent);
    StatusWidgetComponent->SetRelativeLocation(StatusWidgetOffset);
    StatusWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
    StatusWidgetComponent->SetDrawAtDesiredSize(false);
    StatusWidgetComponent->SetDrawSize(FIntPoint(FMath::RoundToInt(StatusWidgetDrawSize.X), FMath::RoundToInt(StatusWidgetDrawSize.Y)));
    StatusWidgetComponent->SetPivot(FVector2D(0.5f, 1.0f));
    StatusWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    StatusWidgetComponent->SetGenerateOverlapEvents(false);
    StatusWidgetComponent->SetWidgetClass(UEnemyStatusBarWidget::StaticClass());
    StatusWidgetComponent->RegisterComponent();
    StatusWidgetComponent->SetHiddenInGame(true);

    if (UEnemyStatusBarWidget* StatusWidget = Cast<UEnemyStatusBarWidget>(StatusWidgetComponent->GetUserWidgetObject()))
    {
        if (CachedStats.IsValid())
        {
            StatusWidget->SetHealthValues(CachedStats->CurrentHealth, CachedStats->MaxHealth);
        }

        StatusWidget->SetFocused(false);
    }
}

void UEnemyCombatDisplayComponent::UpdateFocusedState()
{
    if (!StatusWidgetComponent)
    {
        return;
    }

    UTargetLockComponent* PlayerTargetLock = ResolvePlayerTargetLock();
    const bool bIsFocused = PlayerTargetLock && PlayerTargetLock->GetLockTarget() == GetOwner();
    const double WorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;

    if (UEnemyStatusBarWidget* StatusWidget = Cast<UEnemyStatusBarWidget>(StatusWidgetComponent->GetUserWidgetObject()))
    {
        StatusWidget->SetFocused(bIsFocused);
    }

    UpdateVisibility(bIsFocused, WorldTime);
}

void UEnemyCombatDisplayComponent::UpdateVisibility(bool bIsFocused, double WorldTime)
{
    if (!StatusWidgetComponent)
    {
        return;
    }

    const bool bRecentlyHit = (WorldTime - LastHitDisplayTime) <= VisibleAfterHitDuration;
    const bool bShouldShow = bShowHealthBar && (!bShowOnlyWhenRelevant || bIsFocused || bRecentlyHit);
    StatusWidgetComponent->SetHiddenInGame(!bShouldShow);
}

void UEnemyCombatDisplayComponent::SpawnDamageNumber(float DamageApplied, const FVector& HitLocation)
{
    if (!CachedCharacter.IsValid() || DamageApplied <= 0.0f)
    {
        return;
    }

    USceneComponent* AttachParent = CachedCharacter->GetMesh() ? static_cast<USceneComponent*>(CachedCharacter->GetMesh()) : CachedCharacter->GetRootComponent();
    if (!AttachParent)
    {
        return;
    }

    UWidgetComponent* DamageWidgetComponent = NewObject<UWidgetComponent>(GetOwner());
    if (!DamageWidgetComponent)
    {
        return;
    }

    FVector RelativeLocation = DamageNumberOffset;
    if (!HitLocation.IsNearlyZero())
    {
        RelativeLocation = CachedCharacter->GetActorTransform().InverseTransformPosition(HitLocation) + DamageNumberOffset;
    }

    RelativeLocation.X += FMath::FRandRange(-12.0f, 12.0f);
    RelativeLocation.Y += FMath::FRandRange(-12.0f, 12.0f);
    RelativeLocation.Z += FMath::FRandRange(-4.0f, 10.0f);

    DamageWidgetComponent->SetupAttachment(AttachParent);
    DamageWidgetComponent->SetRelativeLocation(RelativeLocation);
    DamageWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
    DamageWidgetComponent->SetDrawAtDesiredSize(false);
    DamageWidgetComponent->SetDrawSize(FIntPoint(FMath::RoundToInt(DamageWidgetDrawSize.X), FMath::RoundToInt(DamageWidgetDrawSize.Y)));
    DamageWidgetComponent->SetPivot(FVector2D(0.5f, 0.5f));
    DamageWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    DamageWidgetComponent->SetGenerateOverlapEvents(false);
    DamageWidgetComponent->SetWidgetClass(UDamageNumberWidget::StaticClass());
    DamageWidgetComponent->RegisterComponent();

    float LifetimeSeconds = 0.8f;
    if (UDamageNumberWidget* DamageWidget = Cast<UDamageNumberWidget>(DamageWidgetComponent->GetUserWidgetObject()))
    {
        DamageWidget->SetDamageValue(DamageApplied);
        LifetimeSeconds = DamageWidget->GetLifetimeSeconds();
    }

    if (UWorld* World = GetWorld())
    {
        TWeakObjectPtr<UWidgetComponent> WeakWidgetComponent(DamageWidgetComponent);
        FTimerHandle CleanupHandle;
        World->GetTimerManager().SetTimer(
            CleanupHandle,
            FTimerDelegate::CreateWeakLambda(this, [WeakWidgetComponent]()
            {
                if (WeakWidgetComponent.IsValid())
                {
                    WeakWidgetComponent->DestroyComponent();
                }
            }),
            LifetimeSeconds,
            false);
    }
}

UTargetLockComponent* UEnemyCombatDisplayComponent::ResolvePlayerTargetLock()
{
    if (CachedPlayerTargetLock.IsValid())
    {
        return CachedPlayerTargetLock.Get();
    }

    APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
    if (!PlayerPawn)
    {
        return nullptr;
    }

    CachedPlayerTargetLock = PlayerPawn->FindComponentByClass<UTargetLockComponent>();
    return CachedPlayerTargetLock.Get();
}
