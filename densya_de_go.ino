//====================================================================
//  電車でGO!コントローラを用いた鉄道模型制御器
//  Arduino
//  2016.09.18
//====================================================================

//====================================================================
//  概略仕様
//====================================================================

//====================================================================
//  事前処理
//====================================================================

//--------------------------------------------------------------------
//  ヘッダファイル
//--------------------------------------------------------------------
// Gyokimae氏の自作ライブラリ(PSコントローラ用)
// http://pspunch.com/pd/article/arduino_lib_gpsx.html
#include <GPSXClass.h>

//--------------------------------------------------------------------
//  定義マクロ
//--------------------------------------------------------------------

#define PWMOUT 3
#define PWMIN  4

//====================================================================
//  大域宣言
//====================================================================

//---- ハンドルポジション
// Br1はB1であるが、何故かコンパイルエラーになるのでB1のみBr1と記述する。
// 5ノッチ,4ノッチ ... ノッチオフ,1ブレーキ,2ブレーキ ... 8ブレーキ,非常ブレーキ
enum Position { N5, N4, N3, N2, N1, OFF, Br1, B2, B3, B4, B5, B6, B7, B8, EB };

//---- ノッチの構造体(ノッチとは車で言うアクセルのこと)
struct Notch
{
    int n1;
    int n2;
    int n3;
    int n4;
};

//---- ブレーキの構造体
struct Brake
{
    int b1;
    int b2;
    int b3;
    int b4;
};

// スピード
double speed = 0.0;

//--------------------------------------------------------------------
//  関数宣言
//--------------------------------------------------------------------

// ハンドルポジションの格納
Position button_down(void);
// ハンドル位置の返却(ノッチの構造体, ブレーキの構造体)
Position handle_pos( struct Notch n, struct Brake b );
// 速度の返却(ハンドルの位置, スピード)
double return_speedVal( Position pos, double speed );

//====================================================================
//  セットアップ関数(起動時に1度だけ実行)
//====================================================================

void setup()
{
    // シリアル通信 9600bps
    Serial.begin(9600);
    // モード設定 PAD1を使用 アナログモード モードロック
    PSX.mode(PSX_PAD1, MODE_ANALOG, MODE_LOCK);
    // モーターの無効化
    PSX.motorEnable(PSX_PAD1, MOTOR1_DISABLE, MOTOR2_DISABLE);

    // Poll current state once.
    PSX.updateState(PSX_PAD1);
    // 3ピンからpwm出力
    pinMode( PWMOUT, OUTPUT );
    // 4ピンは入力
    pinMode( PWMIN, OUTPUT );
}

//====================================================================
//  本体関数
//====================================================================


void loop()
{
    // ハンドルポジション
    Position pos;
    // ハンドルポジションの取得
    pos = button_down();
    // ハンドルポジションの表示
    Serial.print(pos);
    Serial.print(" ");

    // ハンドル位置からスピードの増減を計算
    speed = return_speedVal( pos, speed );
    // スピードの出力
    Serial.println(speed);

    // pwm出力
    analogWrite( PWMOUT, speed );
    digitalWrite( PWMIN, LOW );

    delay(100);
}


//====================================================================
//  関数定義
//====================================================================

//--------------------------------------------------------------------
//  ハンドルのビットの格納
//--------------------------------------------------------------------

Position button_down()
{
    // ノッチ型変数の宣言
    Notch notch;
    // ブレーキ型変数の宣言
    Brake  bra;
    // ハンドルポジション
    Position pos;
    // ハンドルポジションの取得の開始(これを書かないと取得不可)
    PSX.updateState(PSX_PAD1);

    // ノッチ1
    if ( IS_DOWN_LEFT(PSX_PAD1) )
    {
        notch.n1 = 1;
    }
    else
    {
        notch.n1 = 0;
    }

    // ノッチ2
    if ( IS_DOWN_DOWN(PSX_PAD1) )
    {
        notch.n2 = 1;
    }
    else
    {
        notch.n2 = 0;
    }

    // ノッチ3
    if ( IS_DOWN_RIGHT(PSX_PAD1) )
    {
        notch.n3 = 1;
    }
    else
    {
        notch.n3 = 0;
    }

    // ノッチ4
    if ( IS_DOWN_TRIANGLE(PSX_PAD1) )
    {
        notch.n4 = 1;
    }
    else
    {
        notch.n4 = 0;
    }

    //---- ブレーキ ----
    // ブレーキ1
    if ( IS_DOWN_R1(PSX_PAD1) )
    {
        bra.b1 = 1;
    }
    else
    {
        bra.b1 = 0;
    }

    // ブレーキ2
    if ( IS_DOWN_L1(PSX_PAD1) )
    {
        bra.b2 = 1;
    }
    else
    {
        bra.b2 = 0;
    }

    // ブレーキ3
    if ( IS_DOWN_R2(PSX_PAD1) )
    {
        bra.b3 = 1;
    }
    else
    {
        bra.b3 = 0;
    }

    // ブレーキ4
    if ( IS_DOWN_L2(PSX_PAD1) )
    {
        bra.b4 = 1;
    }
    else
    {
        bra.b4 = 0;
    }
    // マスコンとブレーキの構造体渡し
    pos = handle_pos( notch, bra );
    // ハンドルポジションの返却
    return pos;

}

//--------------------------------------------------------------------
//  ハンドル位置の返却
//--------------------------------------------------------------------

Position handle_pos( struct Notch n, struct Brake b )
{
    // 中途半端なハンドルポジションを取得させないためにstaticを用いる
    static Position pos;
    // ブレーキが解除のときのみ、マスコンの処理に入る
    if ( b.b1 == 1 && b.b2 == 0 && b.b3 == 1 && b.b4 == 1 )
    {
        // ノッチOFF
        if ( n.n1 == 1 && n.n2 == 1 && n.n3 == 1 && n.n4 == 0 )
        {
            pos = OFF;
        }
        // 1ノッチ
        else if ( n.n1 == 0 && n.n2 == 1 && n.n3 == 1 && n.n4 == 1 )
        {
            pos = N1;
        }
        // 2ノッチ
        else if ( n.n1 == 0 && n.n2 == 1 && n.n3 == 1 && n.n4 == 0 )
        {
            pos = N2;
        }
        // 3ノッチ
        else if ( n.n1 == 1 && n.n2 == 1 && n.n3 == 0 && n.n4 == 1 )
        {
            pos = N3;
        }
        // 4ノッチ
        else if ( n.n1 == 1 && n.n2 == 1 && n.n3 == 0 && n.n4 == 0 )
        {
            pos = N4;
        }
        // 5ノッチ
        else if ( n.n1 == 0 && n.n2 == 1 && n.n3 == 0 && n.n4 == 1 )
        {
            pos = N5;
        }
    }
    else    // ブレーキが掛けられていたらこの処理に入る
    {
        // ブレーキ1
        if ( b.b1 == 1 && b.b2 == 1 && b.b3 == 1 && b.b4 == 0 )
        {
            pos = Br1;
        }
        // ブレーキ2
        else if ( b.b1 == 1 && b.b2 == 0 && b.b3 == 1 && b.b4 == 0 )
        {
            pos = B2;
        }
        // ブレーキ3
        else if ( b.b1 == 0 && b.b2 == 1 && b.b3 == 1 && b.b4 == 1 )
        {
            pos = B3;
        }
        // ブレーキ4
        else if ( b.b1 == 0 && b.b2 == 0 && b.b3 == 1 && b.b4 == 1 )
        {
            pos = B4;
        }
        // ブレーキ5
        else if ( b.b1 == 0 && b.b2 == 1 && b.b3 == 1 && b.b4 == 0 )
        {
            pos = B5;
        }
        // ブレーキ6
        else if ( b.b1 == 0 && b.b2 == 0 && b.b3 == 1 && b.b4 == 0 )
        {
            pos = B6;
        }
        // ブレーキ7
        else if ( b.b1 == 1 && b.b2 == 1 && b.b3 == 0 && b.b4 == 1 )
        {
            pos = B7;
        }
        // ブレーキ8
        else if ( b.b1 == 1 && b.b2 == 0 && b.b3 == 0 && b.b4 == 1 )
        {
            pos = B8;
        }
        // 非常ブレーキ
        else if ( b.b1 == 0 && b.b2 == 0 && b.b3 == 0 && b.b4 == 0 )
        {
            pos = EB;
        }
    }
    // ハンドル位置の返却
    return pos;
}

//--------------------------------------------------------------------
//  ハンドル位置に応じてスピードの返却
//--------------------------------------------------------------------

double return_speedVal( Position pos, double speed )
{
    // ハンドルの位置に応じたスピードの返却
    switch ( pos )
    {
        // N5のとき
        case N5:
            speed += 0.5;
            break;

        // N4のとき
        case N4:
            speed += 0.4;
            break;

        // N3のとき
        case N3:
            speed += 0.3;
            break;

        // N2のとき
        case N2:
            speed += 0.2;
            break;

        // N1のとき
        case N1:
            speed += 0.1;
            break;

        // OFFのとき
        case OFF:
            speed += 0.0;
            break;

        // Br1のとき
        case Br1:
            speed -= 0.1;
            break;

        // B2のとき
        case B2:
            speed -= 0.2;
            break;

        // B3のとき
        case B3:
            speed -= 0.3;
            break;

        // B4のとき
        case B4:
            speed -= 0.4;
            break;

        // B5のとき
        case B5:
            speed -= 0.5;
            break;

        // B6のとき
        case B6:
            speed -= 0.6;
            break;

        // B7のとき
        case B7:
            speed -= 0.7;
            break;

        // B8のとき
        case B8:
            speed -= 0.8;
            break;

        // EBのとき
        case EB:
            speed -= 1,0;
            break;
    }
    // pwm出力は0-255の間なのでconstrainで範囲を制限する
    speed = constrain( speed, 0.0, 255.0 );
    // スピードの返却
    return speed;
}
