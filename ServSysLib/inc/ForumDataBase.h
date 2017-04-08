# if ! defined( ForumDataBaseH )
#	define ForumDataBaseH	/* only include me once */

/*
Copyright (c) 2012-2017 MFBS, Fedor Malakhov

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/


#ifndef CommonPlatformH
#include "CommonPlatform.h"
#endif

#ifndef vistypesH
#include "vistypes.h"
#endif

#ifndef SysLibToolH
#include "SysLibTool.h"
#endif

#ifndef SysWebFunctionH
#include "SysWebFunction.h"
#endif

#ifndef WebServInfoH
#include "WebServInfo.h"
#endif

#ifndef UserInfoDataBaseH
#include "UserInfoDataBase.h"
#endif

void ForumDBLoad();
void ForumDbSave();
void ForumDBClear();
void AppendForumMessage(FORUM_MESSAGE_INFO  *MessagePtr);
void AppendForumTopic(FORUN_TOPIC_INFO  *TopicPtr);
FORUM_INFO* ForumDbAddForum();
FORUM_MESSAGE_INFO* ForumDbAddMessage();
FORUN_TOPIC_INFO* ForumDvAddTopic();
void DeleteForumMessage(FORUM_MESSAGE_INFO *RemMsgPtr);
void DeleteForumTopic(FORUN_TOPIC_INFO  *TopicPtr);
void DeleteForumFrt(FORUM_INFO *RemForumPtr);
FORUM_INFO* GetForumByForumId(unsigned int ForumId);
FORUN_TOPIC_INFO* GetTopicByTopicId(unsigned int TopicId);
FORUM_MESSAGE_INFO* GetMessageByMessageId(unsigned int MessageId);
void MessageCountInc(FORUN_TOPIC_INFO *TopicPtr);
void MessageCountDec(FORUN_TOPIC_INFO *TopicPtr);
void LastMessageUpdate(FORUM_MESSAGE_INFO *NewMessagePtr, FORUN_TOPIC_INFO *TopicPtr);
void MsgRemLastMessageUpdate(FORUM_MESSAGE_INFO *RemMessagePtr, FORUN_TOPIC_INFO *TopicPtr,
							 FORUM_MESSAGE_INFO *LastMsgPtr, FORUN_TOPIC_INFO *ChdSkipTopicPtr);
void ForumUserListSort();
//---------------------------------------------------------------------------
#endif  /* if ! defined( ForumDataBaseH ) */
