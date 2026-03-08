
= 章タイトル

//lead{
シンプルなウェブアプリケーションを例に、Chainlitの機能を図を多めに説明します。
//}

== 本章で利用するアプリケーション

本章では、ルールベースで応答するシンプルなウェブアプリケーション @<fn>{support} を例に、Chainlitの機能の説明を行います。アプリ構成は以下の通りで、ユーザーはブラウザからアクセスし、チャットの内容は PostgreSQL データベースに保存されます。

//footnote[support][本章のソースコード全体は以下で公開しています。@<href>{https://github.com/xxx/yyy} （※実際のURLに差し替えてください）]

//image[app_overview][アプリケーション構成図][scale=0.6]{
//}

== ログイン画面

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

== チャット開始画面

//image[chat_start][チャット開始画面][scale=0.6]{
//}

ログイン後のチャット開始画面は @<img>{chat_start} のようになっています。
メッセージ入力欄以外に、アシスタント選択（画像上部）やスターター（画面中央下部）過去のチャット（画像左）などのUI要素があります。また入力欄直下にも歯車や猫のアイコンも存在しています。

このセクションでは、これらのUI要素の機能と実装方法について説明します。

=== スターター（ @<code>{@cl.set_starters} ）

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
//footnote[note-on-chat-profile][利用には、@<hd>{ログイン画面} で述べた認証機能の有効化が必要となります。]

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

ChatGPTと同様に、@<img>{chat_start}の画面左にはチャット履歴が表示され、チャットを再開することができます。

データレイヤーと認証機能を実装することで、チャット履歴の表示と再開が可能となります。

=== コマンド（ @<code>{cl.context.emitter.set_commands} ）

=== チャット設定（ @<code>{cl.ChatSettings} ）

== チャット画面

=== Message（ @<code>{cl.Message} ）

=== Step（ @<code>{cl.Step} ）

=== Action（ @<code>{cl.Action} ）

=== Element（ @<code>{cl.Element} ）

=== Ask User
