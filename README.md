## Intro

**cblog** is web software, the CMS engine in particular. It is aimed at providing content creators, system administrators, and businesses with an easy way to quickly make a website with their content. Despite its name, you can make any CMS-oriented site, like a news site.

The engine is aimed at:

1. Use as few resources as possible.
2. Provide a lightning-fast response time.
3. have the possibility to run on low-power machines (cheap VPS or even MCUs)
4. Making it possible to deploy in a few minutes without additional technical knowledge (currently WIP)
5. Hosting multiple websites on one engine instance

Current state of the project: under active development, a preview version (0.1) with basic functionality is available.


## Build the app

First of all you need to download dependencies.

```bash
git clone https://github.com/xdevelnet/ssb.git
git clone https://github.com/xdevelnet/md4c
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
git clone https://github.com/xdevelnet/cblog
cd cblog
make obj
```

Done! Now you can run your app via freshly created binary.

Use external tools/software to control process.

## Feature availability tables

Application features:

| Feature                                        | Status                                                | Comment                                                                                                                                                                                                                                                                                             |
|------------------------------------------------|-------------------------------------------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| application abstraction                        | Done                                                  | application code is independent from web server or any kind of layer                                                                                                                                                                                                                                |
| supported backend engines                      | Done                                                  | **Mongoose:** allows to compile app and web server to single binary.<br />**Fastcgi (nginx):** Requires web server configuration                                                                                                                                                                    |
| Multiple template support                      | Done, but only one template is enabled                | Abstracted in essb library that were written particularly for this project.<br />Two templates were already done                                                                                                                                                                                    |
| Multiple L10n support                          | Done, but not enabled at all                          | Abstracted in tssb library that were written particularly for this project                                                                                                                                                                                                                          |
| Markdown pages                                 | Done                                                  | When user makes new blog record, app may expect markdown format. If no html provided it will automatically transform markdown into html                                                                                                                                                             |
| Tags support                                   | Mostly done                                           | Tags are displaying on top of each record, engine can show records that are filtered by tag, during adding new record new tag could be added if it's not exist                                                                                                                                      |
| CI<br />Pre-built packages,<br />Docker images | Planned                                               | CI done only for subprojects (ssb)                                                                                                                                                                                                                                                                  |
| Tests                                          | Partially done                                        | Done only for one module, multiple hand tests are available                                                                                                                                                                                                                                         |
| Pages                                          | Done                                                  | Title page with displaying multiple records, filter page with displaying multiple records by specific filter (currently tag filter is supported) with length limiter, single record page with displaying tags, 404 page. 500 page, user login page, logout, user panel page, adding new record page |
| User management                                | Done                                                  | User can login and logout. Each user stores it's id, name, email, hashed password (called "credentials"), user creating time, approve code for email sending, status, time values depending on status. Making new users currently can be done manually                                              |
| User management pages                          | Partially done                                        | Only login/logout is available. User panel is for preview only. Register page and administrator management are planned features.                                                                                                                                                                    |
| RBACL (Role-Based Access Control Lists)        | Planned                                               | Some internal parts are available                                                                                                                                                                                                                                                                   |
| Sessions                                       | Done, currently used mostly for storing user sessions |                                                                                                                                                                                                                                                                                                     |


Supported data storage:

| Engine                             | Status  |
|------------------------------------|---------|
| Files                              | Done    |
| MySQL                              | WIP     |
| SQLite                             | Planned |
| Single file/<br />embedded storage | Planned |


Platform support

| Platform          | Status                            |
|-------------------|-----------------------------------|
| Linux             | Done                              |
| FreeBSD           | Done                              |
| POSIX             | Not tested but will probably work |
| MCUs (like ESP32) | Planned                           |
| Windows           | Small preview is planned          |

External tools/software/references:

| Tool/software    | Description                                                                                                                                                                                                                                                                             |
|------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Nginx            | Web server that were mainly used with fastcgi library.<br />https://nginx.org/                                                                                                                                                                                                          |
| Fastcgi          | Interface that allows to move out from CGI and achieve great performance. Unfortunately, original site is down, only archive left:<br />https://web.archive.org/web/20060221235908/http://fastcgi.com/ <br />Active development is here:<br />https://github.com/FastCGI-Archives/fcgi2 |
| Mongoose         | Web server that could be embedded in application. The easiest way to demonstrate app capabilities                                                                                                                                                                                       |
| ssb (essb, tssb) | library that allows to parse special formatted ssb data in order to have a handy use in this project:<br />https://github.com/xdevelnet/ssb                                                                                                                                             |
| template2essb    | application that transforms html templates to (e)ssb format:<br />https://github.com/xdevelnet/ssb                                                                                                                                                                                      |
| tcsv2tssb        | converter from csv data to (t)ssb format. Mainly used to make text translations.<br />https://github.com/xdevelnet/tcsv2tssb                                                                                                                                                            |


# More

**Why I am doing this?**

1. Modern websites are fat and ugly. They are using too many resources on both sides - on server and on client. In most cases such amount of resources usage is not justified. If you have small business, need single-page website (business card website), or even if you just want a place where you write your thoughts sometimes - there is no reason to host whole relational database just to serve it.
2. I don't like complexity that people have deal with during work with such software. By complexity, I mean not only personal rejection/hostility interpreted programming languages, but effects that they cause, specifically improving for sake of improving, which leads to unnecessary clutter and complexity.
3. As consequence of points 1 and 2, I would like to give people ability to run their web apps on _very_ weak devices (by weak I mean not "bad" but "not powerful"): starting from small VPS (on 128mb RAM WordPress isn't working well), ending by specialized MCU (plugged in PoE ethernet and god ready to use service) and embedded into other apps (like Android).

**Pros and cons**

Pros are marked with green color, cons - red. Points that can't be categorized by pros/cons are marked with blue.

1. Resources. 🟩 Using small amount of resources, especially on some configurations
2. Compatibility. 🟩 Much better compatibility with all possible devices - because of small resource usage it can be embedded into anything
3. Development speed. 🟥️ Very slow. Developing such apps gives no right to make a mistake, therefore developer should deal carefully with that. Wherein, additional techniques to prove reliability and faultlessness/correctness are mandatory.
4. Reliability. 🟦 In theory, even during complicated OS states, such software will continue to function, because in most cases running process already prepared all data and did all preparation operations. So, after serving each request, such data will be not freed, and continue to exist in order to serve further requests. Hardly speaking, if OS is capable to evaluate standard Berkely sockets operations (socket()/send()/recv()/poll()/close()), the application will probably continue to work.
5. Security. 🟦 Despite the fact that compiled software is well known for dangerous vulnerabilities because of developers mistakes, vulnerabilities on websites that are made by using modern tools/ways aren't much better. The whole tragedy is that because of complexity and bloatwareness that's between "received request" and "send a response", such flaws are still exists and found every year, just look at php and WordPress.

**I also have something to say!**

1. I just want to take this opportunity to make a web a little bit better. I am accounting that someone will use this software and as an author I will have DIRECT influence to websites quality.
2. In order to read few text paragraphs and look at small image, I don't want to wait for 3 seconds until page loads on my PC (8 seconds on my flagship Android phone of 2020: Samsung S20), then pressing "Close" button in the corner of frame that just JUMPS ON YOU, hiding all the content and asks for subscribing to VERY IMPORTANT BREAKING NEWS, and then again I need to close another frame that asks me for a cookies. SITE DON'T NEED TO SET ANY COOKIES ON MY BROWSER JUST TO SHOW ME SOME INFORMATION. Thank you. Looking at all of this I just want to open https://motherfuckingwebsite.com/ and enjoy my life being.
3. There would be bundled templates with my app, which servers enough compatibility, but not using much client's resources. In other words, it would be lightweight html without javascript, or just very small amount of javascript which is not required to site to run website.
4. I just want to cull/take away web-monkeys that are making a mess/hell/sodomy in modern web by high entry threshold into C programming language and related technologies. No offence dear colleagues, but business pushes on market, and demand on crap creates supply.
5. I have a mental trauma after php (while I was freelancing at the dawn of a career), which I attempted to cure with perl (during work on PortaOne Inc.), then I finished myself by using Python (during making some help to friends with their projects). These programming languages themselves aren't bad, I am not their hater... Issues that they made and the ways that are solved are just so ugly and causes my rejection. After billions of useless frameworks I just spat and left into Embedded.
