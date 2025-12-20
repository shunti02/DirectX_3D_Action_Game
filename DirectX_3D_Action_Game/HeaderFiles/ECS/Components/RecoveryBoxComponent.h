/*===================================================================
//ファイル:RecoveryBoxComponent.h
//概要:回復判定であることを示すタグ＆データ
=====================================================================*/
#pragma once

struct RecoveryBoxComponent {
    int ownerID = -1;
    int healAmount = 0;
    float lifeTime = 0.5f;
};
