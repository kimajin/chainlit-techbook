
= Chainlitの`@step`、1行変えるだけで画面がガラッと変わる話

> **この記事について**
> ChainlitのUIを触りながら「あれ、消えた？」「なんか増えてる？」を繰り返した記録です。
> Chainlit未経験の方でも読めるように書きます。

== そもそもChainlitって何？

ChatGPTみたいなチャットUIを、Pythonだけでサクッと作れるフレームワークです。
LLMアプリを作るとき「フロントエンドめんどくさい…」という気持ちを救ってくれます。

インストールはこれだけ。パッケージ管理には `uv` を使います：

//emlist[package管理]{
uv init my-app
cd my-app
uv add chainlit
uv run chainlit run app.py
//}

ブラウザが開いて、いきなりチャット画面が出てきます。感動。

== `@step`って何をする人？

Chainlitには **Step（ステップ）** という概念があります。
処理の「中間状態」をUIに表示するための仕組みで、LLMが裏で何をしているかをユーザーに見せたいときに使います。

基本的な使い方はこんな感じ：

//emlist[基本的な使い方]{
import asyncio
import chainlit as cl

@cl.on_message
async def main(message: cl.Message):
    await process(message.content)

@cl.step
async def process(content: str):
    # ここの処理がUIにステップとして表示される
    await asyncio.sleep(1)
    return f"処理完了: {content}"
//}

実行すると、チャット画面に「process」というラベルがついた折りたたみUIが現れます。
「あ、裏でこういう処理が走ってるんだ」が一目でわかる。かっこいい。

== 実験① `show_input=False` にすると入力が消える

デフォルトのstepでは、関数に渡した引数（入力値）もUIに表示されます。
試してみるとこんな感じ：

//emlist[デフォルト（show_input=True）]{
@cl.step  # デフォルト = show_input=True
async def process(content: str):
    return "完了"
//}

//emlist[UI表示]{
▼ process
  入力: "こんにちは"     ← これが見える
  出力: "完了"
//}

で、ここを `show_input=False` にすると：

//emlist[show_input=False]{
@cl.step(show_input=False)  # ← これだけ変える
async def process(content: str):
    return "完了"
//}

//emlist[UI表示]{
▼ process
  出力: "完了"           ← 入力が消えた！
//}

**え、消えた。**

最初に見たとき「あ、バグかな」と思いました。バグじゃなかった。
ユーザーに「何を入力したか」を見せたくない場合や、入力が長すぎてUIが汚くなるときに便利です。

== 実験② stepをネストすると親子関係になる

stepの中でさらにstepを呼ぶと、UIが階層構造になります。

//emlist[ネストしたstep]{
@cl.on_message
async def main(message: cl.Message):
    await parent_step(message.content)

@cl.step
async def parent_step(content: str):
    result = await child_step(content)  # 子stepを呼ぶ
    return f"親の結果: {result}"

@cl.step
async def child_step(content: str):
    return f"子の結果: {content}"
//}

UIではこうなります：

//emlist[UI表示]{
▼ parent_step
  ▼ child_step        ← 子として入れ子になる
    出力: "子の結果: ..."
  出力: "親の結果: ..."
//}

これが面白くて、**どこからstepを呼ぶかで親子関係が変わります。**

//emlist[フラットな呼び出し]{
@cl.on_message
async def main(message: cl.Message):
    await parent_step(message.content)
    await child_step(message.content)  # ← 親の外から呼んだら？
//}

こうするとchild_stepは親を持たない**独立したstep**として表示されます。
同じ関数なのにUIの見え方が変わる。呼び出し元がコンテキストになるんですね。

== 実験③ stepの中でストリーミングして途中経過をリアルタイムに流す

LLMのレスポンスを少しずつ表示する「ストリーミング」、Chainlitでも再現できます。

//emlist[ストリーミングなし]{
import asyncio
import chainlit as cl

@cl.step
async def generate_text():
    # ストリーミングなし（全部終わってから表示）
    result = ""
    for i in range(5):
        await asyncio.sleep(0.5)
        result += f"チャンク{i} "
    return result
//}

これだと処理が全部終わってから一気に表示されます。ユーザーは待つだけ。

Chainlit 2.x では、`cl.Step` をコンテキストマネージャとして使い `stream_token()` を呼ぶことでリアルタイム更新できます：

//emlist[コンテキストマネージャでストリーミング]{
import asyncio
import chainlit as cl

@cl.on_message
async def main(message: cl.Message):
    await generate_text()

async def generate_text():
    async with cl.Step(name="✍️ 生成中") as step:
        for i in range(5):
            await asyncio.sleep(0.5)
            await step.stream_token(f"チャンク{i} ")  # 随時UIに流す
//}

こうするとチャンクが届くたびにUIが更新されます。
「おお、生成されてる…」という体験ができてテンション上がります。

LLMのAPIと組み合わせるとこんな感じ：

//emlist[LLMストリーミング]{
import chainlit as cl

async def call_llm(prompt: str):
    async with cl.Step(name="🤖 LLM生成中") as step:
        # OpenAI等のストリーミングAPIと組み合わせる
        async for chunk in llm_stream(prompt):
            await step.stream_token(chunk)
//}

`@cl.step` デコレータ版と `async with cl.Step()` のコンテキストマネージャ版、どちらも使えますが、**ストリーミングが必要なときはコンテキストマネージャ版**を使うのが Chainlit 2.x での推奨スタイルです。ユーザーが「考えてる感」を感じられるUIになります。

== 実験④ 同期 vs 非同期で見た目が変わる

Chainlitは基本的に非同期（async/await）前提ですが、同期関数にも`@step`は使えます。

//emlist[同期バージョン]{
# 同期バージョン
@cl.step
def sync_process(content: str):
    import time
    time.sleep(2)
    return "同期完了"
//}

//emlist[非同期バージョン]{
# 非同期バージョン
@cl.step
async def async_process(content: str):
    await asyncio.sleep(2)
    return "非同期完了"
//}

どちらも動きます。ただ、**同期関数を使うとイベントループをブロックします。**

複数のユーザーが同時に使うケースでは、同期処理が走っている間は他のリクエストを処理できません。
開発中やサクッと動かしたいときは同期でOK。本番に近い環境では非同期が安心です。

UIの見え方自体はほぼ同じですが、実際の挙動は別物。
「見た目は同じなのに中身が違う」系のトラブルは踏んでから気づくやつです（踏みました）。

== 実験⑤ stepにアイコン・名前をつけて見た目を整える

デフォルトだとstepのラベルは関数名がそのまま出ます。`process` とか `call_llm` とか、英語のままだとユーザーにはちょっと不親切です。

`name` と `type` を指定するとUIの見え方が変わります：

//emlist[nameとtypeを指定]{
import asyncio
import chainlit as cl

@cl.step(name="🔍 キーワードを抽出中", type="tool")
async def extract_keywords(text: str):
    await asyncio.sleep(1)
    # 本来はLLMでキーワード抽出する処理
    keywords = ["Python", "Chainlit", "LLM"]
    return ", ".join(keywords)

@cl.step(name="📝 レポートを生成中", type="llm")
async def generate_report(keywords: str):
    await asyncio.sleep(1)
    return f"以下のキーワードを含むレポートを生成しました: {keywords}"
//}

`type` に指定できる主な値はこちら：

| type | 見た目のニュアンス |
|---|---|
| `"tool"` | ツール・外部API呼び出し的な処理 |
| `"llm"` | LLMへのリクエスト |
| `"embedding"` | Embedding生成 |
| `"retrieval"` | RAGの検索ステップ |
| `"run"` | 汎用の実行ステップ |

名前に絵文字を入れるだけでUIがかなり賑やかになります。最初に試したとき「こんな細かいところまで気が利いてる」と思いました。

== 実践：複数エージェントが協調するマルチstep構成

ここからは少しまとまったユースケースで動かしてみます。

**シナリオ：ユーザーが「〇〇について調べて」と入力すると、複数のエージェントが分担して回答する**

処理の流れはこうです：

//emlist[処理の流れ]{
ユーザー入力
  └─ 🧭 プランナー：何を調べるか分解する
       ├─ 🔍 リサーチャー：各トピックを調べる（並列）
       └─ ✍️ ライター：結果をまとめて回答する
//}

完全に動くサンプルがこちら（LLM呼び出し部分はモックにしています）：

//emlist[マルチstep構成のサンプル]{
import asyncio
import chainlit as cl


# ────────────────────────────────
# エージェント定義
# ────────────────────────────────

@cl.step(name="🧭 プランナー", type="tool", show_input=False)
async def planner(query: str) -> list[str]:
    """クエリをサブタスクに分解する"""
    await asyncio.sleep(0.8)  # 実際はLLMへのAPI呼び出し
    # モック：クエリを3つのサブトピックに分解したと仮定
    topics = [
        f"{query}の基本概念",
        f"{query}の最新動向",
        f"{query}の実用例",
    ]
    return topics


@cl.step(name="🔍 リサーチャー", type="retrieval")
async def researcher(topic: str) -> str:
    """1つのトピックを調査する"""
    await asyncio.sleep(1.0)  # 実際はWeb検索やRAGの処理
    return f"【{topic}】に関する調査結果（モック）: ここに詳細情報が入ります。"


@cl.step(name="✍️ ライター", type="llm")
async def writer(research_results: list[str]) -> str:
    """調査結果をまとめてレポートを生成する"""
    await asyncio.sleep(1.2)  # 実際はLLMへのAPI呼び出し
    combined = "\n".join(f"- {r}" for r in research_results)
    return f"== まとめレポート\n\n{combined}\n\n以上の情報をもとに回答を生成しました。"


# ────────────────────────────────
# メインハンドラ
# ────────────────────────────────

@cl.on_message
async def main(message: cl.Message):
    query = message.content

    # Step1: プランナーがタスクを分解
    topics = await planner(query)

    # Step2: リサーチャーが並列で各トピックを調査
    # asyncio.gather で並列実行 → UIには複数のstepが同時に走る
    research_results = await asyncio.gather(
        *[researcher(topic) for topic in topics]
    )

    # Step3: ライターが全調査結果をまとめる
    report = await writer(list(research_results))

    # 最終回答をチャットに出力
    await cl.Message(content=report).send()
//}

実行すると、UIにはこんな流れでstepが現れます：

//emlist[UI表示]{
▼ 🧭 プランナー          ← まず分解
▼ 🔍 リサーチャー         ← 3つが並列でほぼ同時に走る
▼ 🔍 リサーチャー
▼ 🔍 リサーチャー
▼ ✍️ ライター            ← 全部終わったらまとめ
[最終回答がチャットに表示]
//}

ここで面白いのが `asyncio.gather` の部分。並列で実行しているので、リサーチャーのstepが3つほぼ同時にUIに出現します。なんか工場のラインみたいでちょっと楽しい。

**`show_input=False` をプランナーにつけた理由**

プランナーへの入力はユーザーの生のクエリそのまま。それをstepに表示してもユーザーからすれば「さっき自分で入力したのになぜまた？」となるので非表示にしています。こういう細かい「何を見せて、何を隠すか」の判断もstepの設計で大事な部分です。

**並列実行のときの注意点**

`asyncio.gather` で並列実行すると、各stepは独立したタスクとして走ります。このとき、stepの親子関係は呼び出し元のコンテキスト（`main`関数の中）になるので、リサーチャーのstepはプランナーの子ではなくフラットに並びます。「ネストさせたい」「並列させたい」のどちらを優先するかで設計が変わってくるポイントです。

== 番外編：Claude風の「ウェブを検索しました」UIを再現する

そういえばClaude自身のUIを見ていて気づいたのですが、あの「ウェブを検索しました ∨」って折りたたまれるやつ、まさに `cl.Step` そのものですよね。

あれを真似して作ってみました。

- ゴール：こんなUIを作る

//emlist[目標とするUI]{
┌─────────────────────────────────────┐
│ 🔍 ウェブを検索しました  ∨          │
│   1. [タイトル](URL)                │
│      example.com                    │
│   2. [タイトル](URL)                │
│      example.org                    │
└─────────────────────────────────────┘
┌─────────────────────────────────────┐
│ 🧠 回答を生成中  ∨                  │
│   「〇〇について調べた結果...」     │
│   （文字がリアルタイムで流れる）    │
└─────────────────────────────────────┘
[最終回答がチャットに表示]
//}

折りたたまれたステップの中に検索結果一覧が入っていて、その下でLLMが回答を生成中……というあの体験です。

//emlist[セットアップ]{
uv init chainlit-search-demo
cd chainlit-search-demo
uv add chainlit==2.9.6
//}

//emlist[コード全体]{
"""
Chainlit Step デモ：Claude風のツール表示UIを再現する
"""
import asyncio
import chainlit as cl

# ──────────────────────────────────────────────
# モック検索データ（実際はAPIを叩く部分）
# ──────────────────────────────────────────────

MOCK_RESULTS = [
    {"title": "検索結果 1：サンプルタイトルが入ります", "url": "https://example.com/1", "domain": "example.com"},
    {"title": "検索結果 2：関連する情報ページ",         "url": "https://example.org/2", "domain": "example.org"},
    {"title": "検索結果 3：公式ドキュメント",           "url": "https://docs.example.com", "domain": "docs.example.com"},
    {"title": "検索結果 4：詳細情報はこちら",           "url": "https://example.net/4",  "domain": "example.net"},
]

def _format_results(query: str, results: list[dict]) -> str:
    lines = [f"**検索クエリ:** `{query}`\n"]
    for i, r in enumerate(results, 1):
        lines.append(f"{i}. [{r['title']}]({r['url']})  \n   `{r['domain']}`")
    return "\n".join(lines)


# ──────────────────────────────────────────────
# ステップ定義
# ──────────────────────────────────────────────

@cl.step(name="🔍 ウェブを検索しました", type="tool", show_input=False)
async def web_search(query: str) -> str:
    await asyncio.sleep(0.8)          # 実際はここでSearch APIを叩く
    return _format_results(query, MOCK_RESULTS)


@cl.step(name="🧠 回答を生成中", type="llm", show_input=False)
async def generate_answer(query: str, search_result: str) -> str:
    answer = (
        f"「{query}」について調べた結果をご紹介します。\n\n"
        "複数の情報源を確認し、内容を統合しました：\n\n"
        "- 最新情報が複数メディアで報告されています\n"
        "- 公式サイトと第三者メディアで内容が一致しています\n"
        "- 詳細は各リンク先をご参照ください"
    )
    # cl.context.current_step でステップ内ストリーミング
    current_step = cl.context.current_step
    for char in answer:
        await asyncio.sleep(0.015)    # タイピング感を演出
        await current_step.stream_token(char)
    return answer


# ──────────────────────────────────────────────
# メインハンドラ
# ──────────────────────────────────────────────

@cl.on_message
async def main(message: cl.Message) -> None:
    search_result = await web_search(message.content)
    answer = await generate_answer(message.content, search_result)
    await cl.Message(content=answer).send()
//}

//emlist[実行]{
uv run chainlit run app.py
//}

- ポイント解説

**`show_input=False` で検索クエリを隠す**

ユーザーが入力した文字列をそのままstepの入力欄に表示すると「さっき自分で打ったのに…」となります。`show_input=False` にして、代わりにstepの**出力**側にクエリを埋め込むことで「検索した内容がわかるが、入力の繰り返しにならない」UIになります。

**`type="tool"` と `type="llm"` で視覚的に役割を分ける**

検索ステップには `type="tool"`、LLMステップには `type="llm"` を指定しています。Chainlitのステップ表示では型に応じてアイコンや色合いが微妙に変わるので、ユーザーが「今どの処理が走っているか」を直感的に判断しやすくなります。

**`cl.context.current_step` でステップ内ストリーミング**

`@cl.step` デコレータを使いつつ内部でストリーミングしたい場合、`cl.context.current_step` で現在実行中のステップを取得して `stream_token()` を呼ぶのが 2.x でのやり方です。`@cl.step` の返り値ではなくステップオブジェクトに直接トークンを流すイメージです。



| 変える場所 | 変化 |
|---|---|
| `show_input=False` | 入力値がUIから消える |
| ネストして呼ぶ | 親子の階層UIになる |
| フラットに呼ぶ | 独立したstepとして並ぶ |
| `cl.Step` コンテキストマネージャ + `stream_token()` | 途中結果がリアルタイム更新 |
| 同期関数にする | 動くがループをブロックする |
| `name` + 絵文字 | stepのラベルが読みやすくなる |
| `type` を指定 | 処理の種別がUIで区別できる |
| `asyncio.gather` で並列呼び出し | 複数stepが同時に走って見える |

== 終わりに

`@step` はデコレータ1つで「裏側の処理が見える」体験を作れるChainlitの便利機能です。
引数ひとつ、呼び出し元ひとつで見え方がガラッと変わるのが面白くて、けっこう時間を溶かしました。

LLMアプリのUIとして、ChatGPT的な「考え中…」の可視化にとても向いています。
まずは `uv add chainlit` から試してみてください。

*環境: Python 3.13.4 / Chainlit 2.9.6 / パッケージ管理: uv*
