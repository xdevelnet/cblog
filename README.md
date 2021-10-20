# cblog

## Introduction

**cblog** is my first serious attempt to make web software (you may call it "websites" or backend software) written in non popular way. There is a list of reason why I would like to do that and what are pros/cons of each way to make such software.

**Why I am doing this?**

1. Modern websites are fat and ugly. They are using too much resources on both sides - on server and on client. In most cases such amount of resources usage is not justified. If you have small business, need single-page website (business card website), or even if you just want a place where you write your throughts sometimes - there is no reason to host whole relational database just to serve it.
2. I don't like complexity that people have deal with during work with such software. By complexity I mean not only personal rejection/hostility interpreted programming languages, but effects that they cause, specifically improving for sake of improving, which leads to unnecessary clutter and complexity.
3. As consequence of points 1 and 2, I would like to give people ability to run their web apps on _very_ weak devices (by weak i mean not "bad" but "not powerful"): starting from small VPS (on 128mb RAM wordpress isn't working well), ending by specialized MCU (plugged in PoE ethernet and god ready to use service) and embedded into other apps (like Android).

**Pros and cons**

Pros are marked with green color, cons - red. Points that can't be categorized by pros/cons are marked with blue.

1. Resources. 🟩 Using small amount of resources, especially on some configurations
2. Compatibility. 🟩 Much better compatibility with all possible devices - because of small resource usage it can be embedded into anything
3. Development speed. 🟥️ Very slow. Developing such apps gives no right to make a mistake, therefore developer should be deal carefully with that. Wherein, additional techniques to prove reliability and faultlessness/correctness are mandatory.
4. Reliability. 🟦 In theory, even during complicated OS states, such software will continue to function, because in most cases running process already prepared all data and did all preparation operations. So, after serving each request, such data will be not freed, and continue to exist in order to serve further requests. Hardly speaking, if OS is capable to evaluate standard Berkely sockets operations (socket()/send()/recv()/poll()/close()), the application will probably continue to work.
5. Security. 🟦 Despite the fact that compiled software is well known for dangerous vulnerabilities because of developers mistakes, vulnerabilities on websites that are made by using modern tools/ways aren't much better. The whole tragedy is that because of complexity and bloatwareness that's between "received request" and "send a response", such flaws are still exists and found every year, just look at php and Wordpress.

**I also have something to say!**

1. I just want to take this opportunity to make a web a little bit better. I am accounting that someone will use this software and as an author I will have DIRECT influence to websites quality.
2. In order to read few text paragraphs and look at small image, I don't want to wait for a 3 seconds until page loads on my PC (8 seconds on my flagship Android phone of 2020: Samsung S20), then pressing "Close" button in the corner of frame that just JUMPS ON YOU, hiding all the content and asks for subsribing to VERY IMPORTANT BREAKING NEWS, and then again I need to close another frame that asks me for a cookies. SITE DON'T NEED TO SET ANY COOKIES ON MY BROWSER JUST TO SHOW ME SOME INFORMATION. Thank you. Looking at all of this I just wan't to open https://motherfuckingwebsite.com/ and enjoy my life being.
3. There would be bundled templates with my app, which servers enough compatibility, but not using much client's resources. In other words, it would be lighweight html without javascript, or just very small amount of javascript wich is not required to site to run website.
4. I just want to cull/take away web-monkeys that are making a mess/hell/sodomy in modern web by high entry threshold into C programming language and related technologies. No offence dear colleagues, but business pushes on market, and demand on crap creates supply.
5. I have a mental trauma after php (while I was freelancing at the dawn of a career), which I attempted to cure with perl (during work on PortaOne Inc.), then I finished myself by using Python (during making some help to friends with their projects). These programming languages themselves aren't bad, I am not their hater... Issues that they made and the ways that are solved are just so ugly and causes my rejection. After billions of useleless frameworks I just spat and left into Embedded.

## Build the app

First of all you need to download dependencies.

```bash
git clone http://github.com/xdevelnet/ssb.git
```
If you want to make project run via fastcgi, you would need nginx and fastcgi:
```bash
sudo apt-get install libfcgi-dev nginx
```
If you want to make project run via embedded web server mongoose:
```bash
sudo apt-get install libev-dev
git clone https://github.com/cesanta/mongoose.git
```

Now download project itself and build it. Replace `obj` with `fcgi` or `mon`
```bash
git clone http://github.com/xdevelnet/cblog
cd cblog
make obj
```

Done! Now you can run your app via freshly created binary.

Use external tools/software to controll process.

## Feature tables

Application features:

|Feature|Status|Comment|
|-------|------|-------|
|application abstraction|Done|application code is independent from web server or any kind of layer|
|supported backend engines|Done|**Mongoose:** allows to compile app and web server to single binary.<br />**Fastcgi (nginx):** Requires web server configuration|
|Multiple template support|Done, but only one template is enabled|Abstracted in essb library that were written particularly for this project.<br />Two templates were already done|
|Multiple L10n support|Done, but not enabled at all|Abstracted in tssb library that were written particularly for this project
|Markdown pages|Planned|When user makes new blog record, app expects markdown format. It saves original .md and transforms it to html that will be included during runtime|
|Tags support|Planned|Tags design are already present in my templates|
|CI<br />Pre-built packages,<br />Docker images|Planned|CI done only for subprojects (ssb)|
|Tests|Partially done|Done only for one module|
|Pages|Partially done|Title page with displaying multiple blog records, single record page, 404 page|

Supported data storage:

|Engine|Status|
|------|------|
|Files|Done|
|MySQL|WIP|
|SQLite|Planned|
|Single file/<br />embedded storage|Planned|

External tools/software/references:

|Tool|Description|
|----|-----------|
|Nginx|Web server that were mainly used with fastcgi library.<br />https://nginx.org/|
|Fastcgi|Interface that allows to move out from CGI and achieve great performance. Unfortunately, original site is down, only archive left:<br />https://web.archive.org/web/20060221235908/http://fastcgi.com/ <br />Active development is here:<br />https://github.com/FastCGI-Archives/fcgi2|
|Mongoose|Web server that could be embedded in application. The easiest way to demonstrate app capabilities|
|ssb (essb, tssb)|library that allows to parse special formatted ssb data in order to have a handy use in this project:<br />https://github.com/xdevelnet/ssb|
|template2essb|application that transforms html templates to (e)ssb format:<br />https://github.com/xdevelnet/ssb|
|tcsv2tssb|converter from csv data to (t)ssb format. Mainly used to make text translatios.<br />https://github.com/xdevelnet/tcsv2tssb|
