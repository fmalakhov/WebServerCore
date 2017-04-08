/*!
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

function limitText(limitField, limitCount, limitNum) {
  if (limitField.value.length > limitNum) {
  limitField.value = limitField.value.substring(0, limitNum);
  } else {limitCount.value = limitNum - limitField.value.length;}
}

function isEmail(item) {
var at="@";
var dot=".";
var lat=item.indexOf(at);
var litem=item.length;
var ldot=item.indexOf(dot);
if (item.indexOf(at)==-1) return false;
if (item.indexOf(at)==-1 || item.indexOf(at)==0 || item.indexOf(at)==litem) return false;
if (item.indexOf(dot)==-1 || item.indexOf(dot)==0 || item.indexOf(dot) >= litem - 2) return false;
if (item.indexOf(at,(lat+1))!=-1) return false;
if (item.substring(lat-1,lat)==dot || item.substring(lat+1,lat+2)==dot) return false;
if (item.indexOf(dot,(lat+2))==-1) return false;
if (item.indexOf(" ")!=-1) return false;
return true;
}

function on_sel_show_item_mouseover(imgDocID, textDocId, formName){
  document.getElementById(imgDocID).style.opacity = "0.6";
  document.getElementById(imgDocID).style.cursor = "pointer";
  document.getElementById(textDocId).style.color = "#ff8000";
  document.getElementById(textDocId).style.cursor = "pointer";
}

function on_sel_show_item_mouseout(imgDocID, textDocId, formName){
  document.getElementById(imgDocID).style.opacity = "1.0";
  document.getElementById(imgDocID).style.cursor = "default";
  document.getElementById(textDocId).style.color = "#686868";
  document.getElementById(textDocId).style.cursor = "default";
}

function on_sel_show_item_click(imgDocID, textDocId, formName){
  document.getElementById(imgDocID).style.opacity = "0.6";
  document.getElementById(textDocId).style.color = "#ff8000";
  var SelForm = document.getElementsByName(formName);
  SelForm[0].submit();
}

function AddSessionIdRefLink(ElemId, SessionKey)
{
  ur = document.getElementById(ElemId);
  hr = ur.href;
  hr1 = hr + ';session_id=' + SessionKey;
  ur.href = hr1;
  return true;
}

function on_sel_text_link_mouseover(textDocId){
  document.getElementById(textDocId).style.cursor = "pointer";
  document.getElementById(textDocId).style.color = "#ff8000";
}

function on_sel_text_link_mouseout(textDocId){
  document.getElementById(textDocId).style.cursor = "default";
  document.getElementById(textDocId).style.color = "#ffffff";
}

function on_sel_lang_mouseover(imgDocID, formName){
  document.getElementById(imgDocID).style.opacity = "0.6";
  document.getElementById(imgDocID).style.cursor = "pointer";
}

function on_sel_lang_mouseout(imgDocID, formName){
  document.getElementById(imgDocID).style.opacity = "1.0";
  document.getElementById(imgDocID).style.cursor = "default";
}

function on_sel_lang_click(imgDocID, formName){
  document.getElementById(imgDocID).style.opacity = "0.6";
  var SelForm = document.getElementsByName(formName);
  SelForm[0].submit();
}

function on_spase_resize_mouseover(imgDocID){
  document.getElementById(imgDocID).style.opacity = "0.6";
  document.getElementById(imgDocID).style.cursor = "pointer";
}

function on_spase_resize_mouseout(imgDocID){
  document.getElementById(imgDocID).style.opacity = "1.0";
  document.getElementById(imgDocID).style.cursor = "default";
}

function on_spase_resize_click(imgDocID){
  document.getElementById(imgDocID).style.opacity = "0.6";
  document.SpaseResizeReqForm.screen_width.value = $(window).width();
  document.SpaseResizeReqForm.screen_height.value = $(window).height();
  document.SpaseResizeReqForm.submit();
}

function SetVisibleCenterBlock(BlockDiv)
{
    BlockDiv.style.display='block'; 
    var w = $(window).width();
    var h = $(window).height();
    var divW = $(BlockDiv).width();
    var divH = $(BlockDiv).height();
    BlockDiv.style.position="fixed";
    BlockDiv.style.top = (h/2)-(divH/2)+"px";
    BlockDiv.style.left = (w/2)-(divW/2)+"px";
}
