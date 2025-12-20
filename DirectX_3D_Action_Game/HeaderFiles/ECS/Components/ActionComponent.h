/*===================================================================
//ファイル:ActionComponent.h
//概要:アクション状態（攻撃中フラグ、クールタイムなど）
=====================================================================*/
#pragma once

struct ActionComponent {
    bool isAttacking = false;
    float attackCooldown = 0.0f;
    float duration = 0.5f;
    float cooldownTimer = 0.0f;
};
