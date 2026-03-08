
= Chainlitの基本機能を概観する

//lead{
シンプルなウェブアプリケーションを例に、Chainlitの機能を図を多めに説明します。
//}

== 本章で利用するアプリケーション

本章では、ルールベースで応答するシンプルなウェブアプリケーション @<fn>{support} を例に、Chainlitの機能の説明を行います。アプリ構成は以下の通りで、ユーザーはブラウザからアクセスし、チャットの内容は PostgreSQL データベースに保存されます。

//footnote[support][本章のソースコード全体は以下で公開しています。@<href>{https://github.com/xxx/yyy} （※実際のURLに差し替えてください）]

//image[app_overview][アプリケーション構成図][scale=0.6]{
//}

== ログイン

ユーザーが Chainlit アプリケーションにアクセスすると、ログイン画面が表示されます。（@<img>{login}）
今回は、@<code>{@cl.password_auth_callback} デコレーターを修飾したコールバック関数を定義して、パスワード認証を行っています。

//image[login][ログイン画面][scale=0.6]{
//}

Chainlitの一部の機能は、この認証機能があることではじめて有効となります。例えば、以下の機能が挙げられます。

 * アシスタントの選択（ @<code>{@cl.set_chat_profiles} ）
 * チャット履歴の永続化（ @<code>{@cl.data_layer} ）
 * チャットの再開（ @<code>{@cl.on_chat_resume} ）

Chainlitはその他の認証方法もサポートしています。詳細は Chainlit のドキュメントを参照してください。

 * @<href>{https://docs.chainlit.io/authentication/overview}

認証に成功したら @<code>{cl.User} オブジェクトが返され、チャット開始画面へと遷移します。（認証に失敗したら @<code>{None} を返します。）

== チャット

//image[chat_start][チャット開始画面][scale=0.6]{
//}

ログイン後のチャット開始画面は @<img>{chat_start} のようになっています。
メッセージ入力欄以外に、アシスタント選択（画像上部）やスターター（画面中央下部）過去のチャット（画像左）などのUI要素があります。また入力欄直下にも歯車や猫のアイコンも存在しています。

このセクションでは、チャット画面のこれらのUI要素に関する、基本的な機能と実装方法について説明します。

=== スターター設定（ @<code>{@cl.set_starters} ）

スターターは、ユーザーがチャットを開始する際に選択できるプリセットのメッセージです。スターターをクリックすると対応するメッセージが送信され、チャットが開始されます。

スターターを設定するには、@<code>{@cl.set_starters} デコレーターを使用して、スターターのリストを定義します。個別のスターターは、@<code>{cl.Starter} クラスのインスタンスとして定義します。

//emlist[Starters][python]{
@cl.set_starters
async def set_starters() -> list[cl.Starter]:
    return [
        cl.Starter(
            label="Message",        # 表示されるラベル
            message="Hello World!", # 送信されるメッセージ
        ),
        ...
    ]
//}

スターターは、ユーザーがチャットを開始する際のガイドとして機能し、特に初めてのユーザーにとって便利です。

=== アシスタント選択（ @<code>{@cl.set_chat_profiles} ）

ChatGPTのモデル切り替えと同様の体験で、アシスタントを選択するUIを提供することが可能です。
@<fn>{note-on-chat-profile}
//footnote[note-on-chat-profile][利用には、@<hd>{ログイン} で述べた認証機能の有効化が必要となります。]

//image[chat_profile][Chat Profile][scale=0.6]{
//}


UIの提供は、@<code>{@cl.set_chat_profiles} と @<code>{cl.ChatProfile} を利用して実装します。

//emlist[Chat Profiles][python]{
@cl.set_chat_profile
async def set_chat_profiles() -> list[cl.ChatProfile]:
    return [
        cl.ChatProfile(
            name="Assistant Alice",
            markdown_description="A friendly assistant.",
        ),
        cl.ChatProfile(
            name="Assistant Bob",
            markdown_description="A helpful assistant.",
        ),
    ]
//}

アプリケーション側は、ユーザーが選択したアシスタントを @<code>{cl.user_session.get("chat_profile")} から取得できます。

=== チャット再開（ @<code>{@cl.on_chat_resume} ）

ChatGPTと同様に、@<img>{chat_start}の画面左には、ユーザーが過去に行ったチャット履歴が表示され、チャットを再開することができます。

この機能を利用するためには、認証機能とデータレイヤーを準備し、@<code>{@cl.on_chat_resume} に関する実装を行うという手順を行うこととなります。
後者の実装は、以下の数行で完了します。

//emlist[Chat Profiles][python]{
@cl.on_chat_resume
async def on_chat_resume(_: ThreadDict) -> None:
    pass
//}

==== データレイヤー ( @<code>{@cl.data_layer} )

チャットを再開するための当然の前提として、チャット履歴が保存されている必要があります。
Chainlitはデータレイヤーと呼ばれる抽象化層を通して、チャットデータの保存と取得を行います。
アプリケーション本体はデータベースの種類を直接意識せず、「データレイヤー」に対してデータの保存や取得を依頼するため、PostgreSQLや独自のストレージなどのさまざまな保存先を柔軟に利用することが可能です。

Chainlit はデータレイヤーの API が実装済みの @<code>{SQLAlchemyDataLayer} を提供しており、開発者はデータベースとテーブルを用意するだけで、データレイヤーとして使うことができます。
作成するテーブルは以下のリンク先から確認できます。

 * @<href>{https://docs.chainlit.io/data-layers/sqlalchemy}

特に、@<code>{users}、@<code>{threads}、@<code>{steps}というテーブルに、それぞれユーザー、チャット、チャット内のメッセージを永続化しています。@<fn>{note-on-data-layer}
//footnote[note-on-data-layer][チャットやメッセージは更新と削除が可能であることに対応して、対応するレコードもミュータブルとなっています。特に、イベントをイミュータブルに追加していく構造とはなっていません。]

//image[data_layer][Data Layerの例。 steps テーブルにメッセージが保存されている。][scale=1.0]{
//}

@<code>{SQLAlchemyDataLayer} を使う場合、アプリ側でのデータレイヤーの実装は以下の数行で済みます。

//emlist[Data Layer][python]{
@cl.data_layer
def data_layer() -> SQLAlchemyDataLayer:
    return SQLAlchemyDataLayer(conninfo=os.environ["CHAINLIT_CONNINFO"])
//}

ここで、引数 @<code>{conninfo} には、データベースの接続URIを与えています。

なお、Chainlitの @<code>{BaseDataLayer} を具象化することで、カスタムのデータレイヤーを用意することも可能です。その際に実装する API は以下のリンク先から確認できます。

 * @<href>{https://docs.chainlit.io/api-reference/data-persistence/custom-data-layer}

=== コマンド設定（ @<code>{cl.context.emitter.set_commands} ）

メッセージ入力欄の下にあるボタンは、コマンドと呼ばれる機能です。
ボタンを押すか、Skillsのように「/」から検索して利用することができます。
//image[command_meow_from_input][Command][scale=0.6]{
//}

コマンドを選択したあとに、ユーザーがメッセージを送信することで、対応するコマンドの処理が呼び出されます。

//image[command_wc_output][Word Count コマンドの実行結果][scale=0.6]{
//}

ユーザーが選択したコマンドは、ユーザーの送信した @<code>{cl.Message} の @<code>{command} 属性の文字列から確認でき、アプリケーション側はこの文字列をもとに処理を振り分けます。

//emlist[Command処理の実装][python]{
@cl.on_message
async def on_message(message: cl.Message) -> None:
    if command := message.command:
        match command:
            case "Meow":
                await cl.Message(content="Meow!").send()
            case "Word Count":
                word_count = len(message.content.split())
                await cl.Message(content=f"Word count: {word_count}").send()
        return
    ...
//}

また、コマンドを利用するためには事前にアプリケーション側にコマンドを設定しておくことが必要です。コマンドのメタ情報を @<code>{CommandDict} の辞書として記述し、そのリストを @<code>{cl.context.emitter.set_commands} で送ることで設定します。

//emlist[Commandの設定][python]{
@cl.on_chat_start
async def on_chat_start() -> None:
    ...
    await cl.context.emitter.set_commands(
        [
            {
                "id": "Meow",
                "icon": "cat",
                "description": "Sends a meow message",
                "button": True,     # ボタンを表示するかどうか
                "persistent": True, # 実行後もコマンド選択を維持するか
            },
            {
                "id": "Word Count",
                "icon": "ruler",
                "description": "Counts the number of words in the message",
                "button": True,
                "persistent": True,
            },
        ]
    )
//}

@<code>{icon} ではボタンに利用する Lucide のアイコン名を与えます。

//image[lucide][Lucide( @<href>{https://lucide.dev/} )][scale=0.8]{
//}

スターターと異なり、コマンドはユーザーからのメッセージを追加で与えることが可能です。
また、チャット開始後もコマンドは利用可能です。そのためコマンドを Skills を呼び出す入口として利用するといった使い方ができます。

=== チャット設定（ @<code>{cl.ChatSettings} ）

 @<img>{chat_start} では、コマンドボタンと並列する形で歯車のアイコンが存在しています。
これは、Chat Settings と呼ばれる機能で、チャットの細やかな設定を可能にします。
典型的には、アシスタントが利用するLLMやそのハイパーパラメータの指定を可能とします。

Chat Settings は @<code>{cl.ChatSettings} を介して設定します。
入力するウィジェットにはトグルボタンやスライダーなど様々な種類が用意されており、用途に即して使い分けることができます。

//emlist[Chat Settings][python]{
@cl.on_chat_start
async def on_chat_start() -> None:
    await cl.ChatSettings(
        [
            Select(
                id="Thinking mode",
                label="Thinking mode",
                values=["fast", "slow"],
                initial_index=0,
            ),
            Slider(
                id="Creativity",
                label="Creativity",
                initial=50,
                min=0,
                max=100,
            ),
            Switch(
                id="Enable feature X",
                label="Enable feature X",
                initial=False,
            ),
            Tags(
                id="Interests",
                label="Interests",
                values=["AI", "Machine Learning", "Data Science"],
                initial=["AI", "Data Science"],
            ),
            TextInput(id="Notes", label="Notes", placeholder="Enter your notes here"),
        ]
    ).send()
    ...
//}

ユーザーが歯車のアイコンを押すと設定画面がポップアップされます。

//image[chat_setting][チャット設定画面][scale=0.8]{
//}

設定が更新されると、@<code>{@cl.on_settings_update} に登録した関数が呼び出されます。
設定をユーザーセッションに保存することで、アプリケーション側が設定内容に応じた処理を行うことができます。

//emlist[Chat Settings Update][python]{
@cl.on_settings_update
async def setup_agent(settings: dict[str, Any]) -> None:
    cl.user_session.set("chat_settings", settings)
//}

== メッセージ

チャット内では、ユーザーとアシスタントのメッセージのやり取りが行われます。
ユーザーがメッセージを送信すると、@<code>{@cl.on_message} を修飾したコールバック関数が呼び出され、そのなかでアシスタントの処理と返答が行われます。

//image[command_meow_output][チャット画面][scale=0.8]{
//}

しかし、アシスタントの回答といってもその表示方法は様々です。
例えば、ChatGPTを見ると、最終的な回答メッセージだけではなく、その間の思考やツールの実行内容が表示されていることが見て取れます。
また、Claude Code や Codex などのコーディングエージェントを利用すると、時折、エージェントがユーザにYes/Noを提示して許可で求めたり、選択肢を提示して質問を投げかけることがあります。

このようなアシスタントの意図を表すには、メッセージに関する UI 上の工夫が必要です。
Chainlitでは、これらの幅広さに対応して様々な機能が提供されています。
そこで、この節では、ChainlitのメッセージのUI要素について、基本的な機能と実装方法を説明します。

=== Message（ @<code>{cl.Message} ）

@<code>{cl.Message} はユーザとアシスタントがやり取りする最も基本的なメッセージです。
ユーザの入力したメッセージや、アシスタントの出力した回答がこのクラスのインスタントとして表現されます。

例えば、ユーザーの入力をそのまま出力する場合、実装は以下のようになります。

//emlist[@<code>{cl.Message}][python]{
@cl.on_message
async def on_message(message: cl.Message) -> None:
    await cl.Message(content=f"Received: {message.content}").send()
//}

特に、ユーザーの入力メッセージは関数の引数を通じて与えられ、メッセージの文字列は @<code>{content} 属性にあることや、メッセージの送信は @<code>{send()} で行われることが見て取れます。

@<code>{send()} を呼び出すと、背後では以下のことが行われます。
@<fn>{messagebase-send}
//footnote[messagebase-send][詳細は @<code>{MessageBase.send()} の実装を参照してください。]

 * UI画面へのメッセージ送信（メッセージが表示される）
 * （もしあれば）データレイヤーにメッセージのレコードを追加
 * @<code>{cl.chat_context} にメッセージを追加

最後の @<code>{cl.chat_context} は、チャット内でこれまでやり取りしたメッセージを @<code>{list[cl.Message]} として保存している変数です。
特に、@<code>{cl.chat_context.get()} や @<code>{cl.chat_context.to_openai()} で取得したメッセージの履歴を LLM の入力として与えるといった使い方ができます。

なお、メッセージの内容は @<code>{update()} や @<code>{remove()} を用いて更新・削除することができます。（@<code>{send()} と同様に、背後で更新・削除が行われます。）

//emlist[ @<code>{update()} 及び @<code>{remove()}][python]{
@cl.on_message
async def on_message(message: cl.Message) -> None:
    msg = cl.Message(content="This message will be updated after 2 seconds.")
    await msg.send()

    await cl.sleep(2)

    msg.content = "This message will be removed after 2 seconds."
    await msg.update() # このタイミングでメッセージが更新される

    await cl.sleep(2)
    await msg.remove() # このタイミングでメッセージが削除される
//}

=== Step（ @<code>{cl.Step} ）

LLM アプリケーションでは、AI がどのように回答に至ったのかをユーザに説明したい場面があります。

例えば次のような情報です。

 * 検索ツールを使った
 * データベースを参照した
 * 中間計算を行った

Chainlit では、このような処理過程を可視化するための仕組みとして
@<code>{cl.Step} が用意されています。

使い方を学ぶため、以下の実装と対応する実行結果を見てみましょう。

//emlist[ @<code>{cl.Step}][python]{
@cl.on_message
async def on_message(message: cl.Message) -> None:
    ...
    async with cl.Step(
        name="Step started", # ステップに最初に表示される名前
        default_open=True,
    ) as step:
        await cl.sleep(1)
        ...

        async with cl.Step(         # ステップはネストすることができます。
            name="Tool call",       # 表示されるラベル
            type="tool",            # ステップの種類を表すタグ
            default_open=True,
            show_input="python",    # 入力のシンタックスハイライト
        ) as second:
            step.name = second.name

            second.input = "add(1, 2)"
            await step.update()
            await second.update()
            await cl.sleep(1)
            second.output = {"output": 3}

        step.name = "Step completed"
//}

//image[step][@<code>{cl.Step} の表示][scale=0.8]{
//}

このネストされたステップを実行すると、以下のように途中経過を表示しながら処理が進みます。

 1. 親ステップが @<code>{__aenter__()} し、@<code>{send()} により、「使用中：Step started」という文字列と共にステップが表示される。
 1. 子ステップが @<code>{__aenter__()} し、ネストされた形で同様に表示される。
 1. 子ステップの @<code>{input} と 親ステップの @<code>{name} が更新される。 @<code>{update()} が呼び出されて画面に反映される。
 1. 子ステップの @<code>{output} が更新され、子ステップが @<code>{__aexit__()} する。@<code>{__aexit__()} 内部で @<code>{update()} が呼び出されてUIに反映される。
 1. 親ステップが @<code>{__aexit__()} する。表示は「使用済み：Step completed」となる。

コンテキストマネージャーのため分かりにくいですが、上の処理をみると @<code>{cl.Step} は @<code>{cl.Message} と同様のメソッドを持っていることが分かります。
対応して @<code>{cl.Step} の内容も @<code>{cl.Message} と同様にデータレイヤーの @<code>{steps} テーブルに永続化されます。
一方で、メッセージ履歴を管理する @<code>{cl.chat_context} には反映されません。
ここから、@<code>{cl.Step} が回答過程を表現する機能であるという意図が読み取れます。

=== Action（ @<code>{cl.Action} ）

=== Element（ @<code>{cl.Element} ）

=== Ask User
