/*===================================================================
//ファイル:System.h
//制作日:2025/12/05
//概要:全てのシステムの基底クラス
//制作者:伊藤駿汰
//-------------------------------------------------------------------
//更新履歴:
//2025/12/05:新規作成
=====================================================================*/
#ifndef SYSTEM_H
#define SYSTEM_H
//前方宣言
class World;

class System {
public:
	virtual ~System() = default;

	//初期化時にWorldを受け取る
	virtual void Init(World* world) { pWorld = world; }

	//更新処理
	virtual void Update(float dt){}

	//描画処理
	virtual void Draw(){}

protected:
	//ECS全体の管理者ポインタ
	World* pWorld = nullptr;
};

#endif //SYSTEM_H
