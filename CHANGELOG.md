# Changelog

CM 01/02 Bosman Edition uses semantic versioning. A patch release (0.1.1) fixes bugs. A minor release (0.2.0) adds features. A major release (1.0.0 and later) can break saved games or settings. Releases below 1.0.0 are test releases.

## 0.6.0 (pre-release)

Play any season from 2001/02 to 2025/26 with that season's squads, leagues, cups and rules. In a long game the rules change each July as they did in real life. New lines in `settings.txt`: `season` and `changing_rules`.

- Choose Season (`season`): a screen with a tile for each season from 2001/02 to 2025/26, in rows for the 2000s, 2010s and 2020s. Click the season button at the top of Setup Game to open it. Each season has one database, and its tile starts it, so you never install a database by hand. The details under the tiles show the database and who made it, the European format of the season ("Europe: ...") and the start date. The line "More versions of each season are on the Update Team's forum" has a button that opens champman0102.co.uk.
- A database for every season is in the download: the official 3.9.68 data for 2001/02, 21 databases of the CM 01/02 Update Team, and our own Summer 2002, Summer 2004 and Summer 2006 for 2002/03, 2004/05 and 2006/07. Summer 2002 is the 3.9.68 data with the summer 2002 club moves, releases and contract dates of CM 02/03, and its credit on Choose Season reads "Bosman Edition database, data from CM 02/03". Summer 2004 and Summer 2006 have facts from public sources and ratings by the Update Team, and no person in them appears twice. Each season database is zipped and unpacks into the save folder the first time you pick its season. Unpacked, the Update Team databases take about 1.4 GB together.
- Custom database: a tile for a database folder of your own in `Databases`. You choose the folder and say which season it holds, and that season's rules apply. A `database = 3.9.60` or `3.9.65` line in `settings.txt` still starts that data set.
- A new game starts in July of its season. Every person has the right age and year of birth for that July, and contract, joined and history dates fit the season. For seasons before 2010/11, the competition history lists the real winners from 2001/02 up to the start season.
- Leagues by season: the number of clubs, promotion and relegation places, play-offs and splits of each season, for all divisions of the 27 nations in the game. New divisions at tiers the original already plays, such as the 3. Liga, the Primera Federación, Liga 3 and the single Serie C in Italy, and Brazil's state championships. When a season's database lacks a real top or second tier club, the game adds the club with a squad. Your results decide which clubs go up and down.
- Domestic cups by season: replays or none, single-leg or two-leg ties, group stages, four-club super cups, and cups that start, stop or come back.
- Substitutes, bench size, foreign player rules, work permits and transfer windows follow the season. The Brexit work permit rule applies from 1 January 2021 in England, Scotland, Wales and Northern Ireland. With changing rules on, England has no limit of three non-EU players in a match squad, as in real life.
- European cups of each season: two Champions League group stages until 2002/03, then one group stage and a round of 16. UEFA Cup groups from 2004/05, and the Intertoto Cup until 2008. The Europa League and the Champions and League Paths from 2009/10. The Conference League and no away goals from 2021/22. A league phase of 36 clubs in all three cups from 2024/25. The places of each nation come from that season's UEFA access list.
- International football by year: the UEFA and CONCACAF Nations Leagues, Euro qualifying with Nations League paths, Euros of 24 nations from 2016, the 24-team Africa Cup of Nations and Asian Cup from 2019, the 2022 World Cup in November and December, the 48-team World Cup of 2026 with its qualifying, the Gold Cup and the Finalissima. The Confederations Cup ends, golden goal goes, Australia moves to Asia and nations carry their name of each year.
- Competitions and rounds carry their name of each season, such as the Championship or the Europa League, with no sponsor names.
- Changing rules (`changing_rules`, on by default): set it for each new game with the Changing rules button at the top of Setup Game or on the Choose Season screen. The save keeps it, and a game cannot change it after the start. With it on, each rule changes in the July it changed in real life, also in a game that starts in 2001/02, and a news item at the start of the season lists the changes. After 2025/26 the changes already announced with a date come in, such as the Championship play-offs for 3rd to 8th from 2026/27, and then the rules stay. With it off, the rules of the start season stay for the whole game. The Game tab of Bosman Edition Options sets the default.
- In a language other than English, the game no longer stops a new game or a load with the error box "Database..cpp 7784" when it uses the 3.9.60 or 3.9.65 data, a season database or a save of these. New games on 3.9.60 and 3.9.65 show club, nation, city, competition and stadium names in your language, from the language files of those versions. With a season database or a custom database, a new game shows the names in your language for every club, nation, city, competition and stadium that the 3.9.68 language files hold, and the save keeps them. Competitions that 0.6.0 adds and clubs that are new in a later database keep their English names. Older saves on 3.9.60 or 3.9.65 keep the English names.
- In a language other than English, a load and a new game no longer show the box "CD Not Required" in that language, for example "CD Non Richiesto".
- Seasons are written in full, for example "2009/10" in place of the original's "2009/0".
- Saves from 0.5.0 and earlier load with changing rules off.
- Setup Game: two new buttons sit at the top. The left one shows the season and its database, and a click opens Choose Season. The right one shows whether the rules change, and a click switches it. Hall Of Fame, Game Credits and Web Sites are on a new Extras screen.
- The start screens have the title "Championship Manager: Bosman Edition".
- Bosman Edition Options: the Screen tab is now called Graphics.
- Attribute groups: a goalkeeper's profile shows One On Ones and Consistency in the Goalkeeping group, and Set Pieces and Technique move into it from the Technical group. Both come from the game's own player data, which the original does not show. Attributes that your scouts do not know show as " -", as the others do.
- Bosman Edition Options: the Save folder row has a Change button that opens your computer's folder picker. The game uses the new folder from the next start. Saved games already in the old folder stay there.
- Team views: Best XI and Squad depth are one screen, in the look of the original game. The pitch lies on its side with a card for each of the 11 places: the Best XI player, then the next two for that place. Half stars, from 0.5 to 5, show how well each player fits the place. Every formation fits on the pitch in classic and widescreen. A player who cannot play the next match has a tag: Inj (injured), Int (away with the national team) or Out (banned, on loan or not allowed by the rules).
- Compare players has the look of the original comparison screen: the higher number of each pair in that player's colour, green or red, an equal pair in yellow, and the polygon on a dark panel.
- HD mode is gone. On a Mac screen it showed bars around the picture and small text, while widescreen fills a large screen with sharp, bigger pixels. A `display = hd` line in `settings.txt` now gives widescreen.
- Display report: if the picture looks wrong, press Ctrl+Shift+D (Cmd+Shift+D on a Mac). The game saves `display-report.txt` in the save folder, puts the same text on the clipboard and shows where the file is. The report lists your system, your screens and their scale, the window size and how the game scales its picture, and the last 10 changes of the window. The game also writes the file at each start and quit. Paths in it show `<user>` in place of your account name.
- Windows: text stays sharp with display scaling at 125 % or 150 %. Before, Windows stretched the whole window and blurred it. A new window opens at the size of the picture, as at 100 %.
- The picture fills the window at any size, larger or smaller, and keeps its shape. Black bars show only where the window has a different shape from the picture. With `sharp_picture` on, letters keep an even shape at every size. With it off, the picture is smoother and softer.
- The window cannot get smaller than the picture: 800x600 in classic mode, 1280x800 in widescreen. On a smaller screen the limit is the largest window that fits.
- Classic mode in full screen (F11, or Cmd+F on a Mac) changes the screen to 800x600, as the original does, when the screen has that mode. Macs have no 800x600 mode, so there the picture fills the screen.
- On a screen less than 800 pixels high, widescreen shows a message once at start, and the Display mode row of Bosman Edition Options says that classic mode suits the screen better.
- Saves from 2 GB up to 4 GB now load. The game wrote a wrong position for every part of a save after 2 GB, so the load stopped at an error box. This is a fault of the original game too. A save above 4 GB is refused: a box says the game is not saved, and your earlier save stays.
- Long games with many leagues no longer run out of memory. A Select All game from 2001 stopped in its ninth season, also in the base game.
- Full player search with every league loaded lists every player again. Before, it said "1 player found" over an empty list.
- The comparison screen fits long names, clubs and positions in their boxes, so no text is cut.
- Nations by season: the Netherlands Antilles become Curaçao from 2011/12, Kosovo plays in UEFA from 2016/17, and an old Yugoslavia record that some databases keep beside Serbia no longer plays, for example at the 2022 World Cup.
- Long test games in every start season, with a save and a load each July, found faults in the new season code. These are fixed in this release: error boxes on match days, in discipline, in staff and database checks and in the draws of European and World Cup qualifying, a Europa League play-off with no second leg, and saved season games that did not load. Among them: national squads short of players after a load, bans after Australia moved to Asia in 2006, a ban held by a player on loan between an Irish and a Northern Irish club in the All-Ireland cup, and empty places in some domestic cups, such as the 2003 Polish Cup and the Leinster and Munster Senior Cups.
- A game that starts after World Cup or Euro qualifying began plays the real finals: the nations that really qualified and the real play-off ties of the World Cups 2006 to 2022 and the Euros 2004 to 2024.
- World Cup qualifying in South America and CONCACAF ends on each cycle's real last matchday, so the play-offs between confederations always have both teams. A game that starts in 2025/26 has the real 2026 finalists and play-off results, and in the 2030 qualifying none of the six hosts plays a qualifier.
- Euro 2028 qualifying has its real play-off paths in March 2028, from the 2026-27 Nations League. The two best-ranked of England, the Republic of Ireland, Scotland and Wales that do not qualify take the host places.
- Bonaire, French Guiana, Guadeloupe, Martinique, Saint-Martin and Sint Maarten stay out of the CONCACAF Nations League, because the databases give them no players. Before, a load could draw them in.
- An under-21 squad no longer takes players from the senior squad, or from any other national squad, that plays in the same international window, so under-21 sides no longer go into a match short of players.
- Outside the base game, the MLS play-offs use extra time and penalties, as real MLS has since 2000. A level league match stays a draw. Before, a play-off shootout could go on until the game stopped with the error box "match_pl..cpp 4885".
- The Champions League and Europa League preliminary rounds play on their real days in late June, with extra time and penalties. Before, a level match was replayed and the winner's first qualifying tie came a week late.
- Europa League clubs register their squads again in February for the knockout rounds. Before, a club that came through the qualifying rounds kept its August squad and could stop the game with only 10 fit players.
- The Italian leagues no longer stop after 2015/16. Before, Serie A and Serie B stayed at 2015/16 and Italy never started the next season.
- The Coppa Italia Serie C fills all 32 places of its knockout round, with the seeds in their places. With changing rules on, it also plays on when its group size changes. Before, two places stayed empty and the cup could stay at one edition.
- With Serie C2 in the game, a Serie C2 group that ends in a later season no longer leaves an empty league that stopped the game with an error box.
- League One keeps 24 clubs in 2019/20. Bury left after the season start, and the division had stayed at 23 for the next seasons, so the FA Cup and the League Cup each lacked a club.
- League Two keeps 24 clubs from 2025/26. Before, a 2025/26 start could give it 20.
- In a 2025/26 start the FA Cup takes the right number of clubs from the National League and below. Before, a first-round tie could have an empty place or a club with a garbled name, and the game stopped with an error box.
- The four-club Supercopa de España: when one club holds two places, the next club in the league takes the second place, as in the real competition. Before, that semi-final never ended and the cup stayed at one edition.
- In a 2002/03 start, the Turkish Category B plays in three regional groups, as in the real season. Each group sends one club up and two down, and clubs keep their group at the season change. The 2002/03 club moves leave the Category B clubs in their groups, and a new club goes to its real group.
- The Leinster and Munster Senior Cups and the Polish Cup pick a fresh list of clubs each season. Before, a club from the last edition could stay in a place, and the cup stopped with the error box "Cup..cpp 1187" or stayed at one edition.
- The Russian Cup starts its new edition when the last final is played after 1 June. Before, it could stay at the old edition for a season.
- The Segunda División play-off is played every season. Before, the game often found no time for it and sent the third club up. The final has extra time and penalties on its second leg, and from 2020/21 its two legs are as far apart as in real life. Before, a level final went to replays and the game stopped with the error box "Cup..cpp 1450".
- The knockout rounds of the Champions League, Europa League and Conference League play on their real dates of each season from 2003/04. Seasons after the last published calendar keep the same weekdays.
- International friendlies are played on the real FIFA days, and the qualifying, group and league-phase matches of the UEFA cups on their real days. UEFA ties are no longer moved because players are away for friendlies, so the draws of the next round are no longer late. The first season of a game that starts in 2001/02 keeps the original game's calendar, so that the base game stays the same.
- World Cup and Euro qualifiers move to the nearest real FIFA match day that has no continental club match. In a game that starts in 2001/02 this begins in the second season.
- The Champions League qualifying draws keep the Champions Path and the League Path apart, as in the real competition, and the play-offs have both paths from 2018/19. The draws are made one path at a time, so they come out in a different order from older builds.
- The Danish leagues start each season on time. Before, a match that international call-ups, a fixture clash or a TV move pushed past the start day made the leagues wait a year.
- With changing rules off, the South group of the Greek Super League 2 starts each season. Before, it stayed in its first season.
- A player on loan keeps his contract with his parent club and stays on loan when his loan club takes him off its staff. Before, the game could stop with the error box "contract_manager..cpp 5941" when the loan ended.
- The club tables of each season point at the right club in 55 places, for example Yimpaş Yozgatspor in the 2002/03 Turkish Category A and Dijon FCO in France.
- A club no longer rotates its best players out of a match when fewer than 25 of its squad can play it. Before, AJ Auxerre could field 7 players in a 2005/06 UEFA Cup match, and the game stopped with an error box.
- When an AI club loans players out, it keeps at least 3 goalkeepers, so its reserve side always has a keeper. Before, a reserve match with no goalkeeper could stop the game with an error box.
- Japan's Emperor's Cup ends each edition and starts the next. Before, a tie made after its date could fall after the next round's match day, and the cup stayed at one edition.
- A Competitions menu with more than 24 competitions, such as Brazil's with its state championships, has a sub menu More for the rest.
- More faults found by the long test games are fixed: error boxes when a national squad named the same player twice, on the news of an EFL Trophy group match, at the 2019 Confederations Cup with changing rules off, at the 2022 Finalissima and in the Portuguese Cup of a 2021/22 start, and a Club World Championship whose knockout never ended after a load.
- A season database copied into the game's own Data folder starts a Select All game without the error box "Database.cpp 1844".
- The Linux download is now `CM0102-Bosman-Edition-<version>-x86_64.AppImage`, and it has the icon file that AppImage tools look for.
- The fix of 0.5.1 is in every download: a game with Brazil selected can save the São Paulo, Gaúcho, Goiás and Paraná state championships.
- The code is under GPL-3.0 from 0.6.0. Releases up to 0.5.0 stay CC0. The function and data maps stay CC BY 4.0.
- Same game: the base game is 2001/02 with the 3.9.68 data and changing rules off. In the base game the season code changes nothing: the same clicks with the season code turned off by a test switch give the same save. The new Setup Game moves the original's memory blocks, so a base game of 0.6.0 is not byte for byte the same as one of 0.5.0.
- Limits in 0.6.0: the bench and the substitutes stay at 9 or fewer, because the match engine holds 9, so a bench of 12 plays as 9. The MLS stays at 12 clubs. Careers that a season database never updated stop in 2001. In databases from September 2009 most players joined their club in 2008. The award history keeps the years of the data. The 2000/01 Portuguese Second League runner-up is Varzim (three clubs ended on 64 points).

## 0.5.0 (pre-release)

An options box in the game, positions on the tactics screen, foreign players in colour, and fixes for rare faults of the original. Each new option has a line in `settings.txt` in the save folder.

- Bosman Edition Options: a box with two tabs, Screen and Game, that shows every option of the Bosman Edition. It opens from Setup Game and from Game Options in the side bar, after Game Settings. A change saves to `settings.txt` at once and keeps your own lines and comments. Display mode, All sound and Save folder take effect at the next start. Save folder is shown, and you change it in `settings.txt`.
- The tactics screen shows each player's position in the Condition view, before and during a match, in classic, widescreen and HD (`match_columns`).
- Foreign players in colour (`foreign_colours`): on the squad and tactics screens, the names of players who count against the foreign player limit of the club's next league or cup match are light violet. It works for every club. The game's "Fgn" status stays.
- Full player search (`full_player_search`, off by default): the player search lists every player, also players your manager does not know. With it on, a search changes later days, as opening a screen does.
- Fixes for rare faults of the original game: a news item with no text, some transfer offers, squad updates, staff records after a part-exchange deal, the competition table and the World Cup draw. The original crashed on these. The port went on with wrong data. Now it skips the bad item.
- The game no longer uses a full processor core while it waits for input (about 1% in place of 100%). Matches and holidays play the same.
- The Database button shows 3.9.68, 3.9.60 and 3.9.65 first, then your databases from the oldest season to the newest, each with its season and folder name, for example "2025/26 (May 2026)". In classic mode a long folder name shows the season alone. A folder whose name has no month and year comes last.
- The intro shows the CM 01/02 logo with every database, and a database in `Databases` no longer replaces the pictures of the intro.
- A new Bosman Edition logo, on the site and in the intro.

## 0.4.0 (pre-release)

New screens and an HD layout. Each new option has a line in `settings.txt` in the save folder. No feature changes the game's decisions while you make the same clicks.

- Team views on every squad screen, from the Assistant menu (`squad_views`): your best XI on a pitch in any of the game's formations, with the subs and other players beside it, and a squad depth table with the best players for each position. The numbers are the assistant's score for each position, from the attributes the profile shows. Players who cannot play the next match are left out of the eleven and shown in grey in the table.
- A player comparison with a polygon of eight strengths, on the original's comparison screen and on the Compare tab. Two goalkeepers get goalkeeper strengths.
- Attribute groups on the player profile (`attribute_groups`): Technical, Mental and Physical, with Goalkeeping first for goalkeepers.
- Print to PDF: Print To shows PDF in place of Printer, and each print saves a PDF in the `Prints` folder of the save folder. File still saves a text file, and Both saves both.
- An HD layout at 1920 × 1080 (`display = hd`). A new window shrinks to fit the screen.
- The CM 01/02 Update Team's databases from 2003 to 2026: copy a database into a folder in `Databases` in the save folder, and choose it with the Database button. The game still starts in July 2001.
- Message boxes are centred in widescreen and HD.
- The box "CD Not Required" no longer shows after a new game or a load.
- A clickable name no longer shows its underline under a box that covers it.

## 0.3.1 (pre-release)

A fix for the Assistant menu on the squad screen.

- Pick best team, Pick most match-fit team and Pick under-21 team now score each player by the key attributes of the position, as the profile shows them. For example, a goalkeeper is scored on Handling, Reflexes, Agility, Positioning and Jumping. The score also counts how well the player fits the position.
- The assistant finds the best eleven as a whole. In 0.3.0 it filled one position at a time, so a natural right back could sit on the bench while a centre back played at right back.
- A player plays out of position only when no free player suits that position.

## 0.3.0 (pre-release)

New features and a Linux version. Each option has a line in `settings.txt` in the save folder. No feature changes a match result or a decision of the game, except the team that you or your assistant pick.

- Linux: one AppImage for 64-bit Linux. It also runs on the Steam Deck in desktop mode.
- A Database choice on the Setup Game screen for each new game: the original 2001/02 data (3.9.60), the 2001/02 December update (3.9.65) or the 2002/03 data (3.9.68, the default). The `database` line holds the choice.
- An Assistant menu on the squad screen picks the best team, the most match-fit team or an under-21 team. It weighs each player's current ability and how well the player fits each position.
- Holiday until next match: an item below Go on Holiday ends the holiday on the day before your club's next match.
- A Fastest commentary speed, twice as fast as Very Fast. As in the original game, the commentary speed can change how a match plays out.
- Save Game suggests a name with your club and the game date, for example "Arsenal 2001-07-20".
- The game keeps the last three copies of each save in a `backups` folder next to your saves (`save_backups`).
- The window opens at the size and place it had when you quit, in full screen if it was (`remember_window`).
- Condition and morale have colours: red below 75%, amber from 75 to 89% and green from 90% (`condition_colours`).
- Windows: Ctrl+[ is Back and F11 switches full screen.
- Fixes: the underline in the news covers the whole name, and the "Replace this file?" text is centred in widescreen.

Thanks to u/gundenaf on Reddit, who asked for the Linux version and the database choice.

## 0.2.0 (pre-release)

Comfort features and sound. Each feature has a line in `settings.txt` in the save folder, and `off` turns it off. No feature changes a match result or a decision of the game.

- Sound: the game's own sounds play. The game is silent while its window is not in front. `sound = off` mutes it.
- The space bar presses Continue Game.
- Back: Cmd+[ on a Mac, or the back button of a mouse. Escape closes a popup. Cmd+F switches full screen on and off.
- The picture scales in whole pixels, so text stays sharp.
- `save_folder` keeps saved games in another folder, for example in iCloud Drive.
- Player and club names show a hand and an underline under the mouse. A click opens the profile.
- Attribute numbers have colours by value: 1 to 5 red, 6 to 10 amber, 11 to 15 light green, 16 to 20 bright green.
- In a match, widescreen only: the tactics screen shows each player's positions and condition beside the match rating, and the Player Ratings view shows the condition of both teams.
- Days pass faster while you are on holiday.
- Widescreen fixes: longer texts fit, and the name buttons in the news land on the names.

## 0.1.0 (pre-release)

The first test release.

- Championship Manager 01/02 at version 3.9.68: the 2008 free release with the last official patch. No fan patches or fan databases.
- Mac: one app for Apple silicon and Intel, macOS 11 or later.
- Windows: an installer for 64-bit Windows 10 and 11, with Start menu and desktop shortcuts.
- Widescreen at 1280 × 800 by default. The window can be resized and keeps the shape of the picture.
- The old 800 × 600 layout: open `settings.txt` in the save folder and change the line to `display = classic`.
- Saves and settings live in `~/Library/Application Support/cm0102` on a Mac and in `%APPDATA%\cm0102` on Windows.

Not in this release: sound, network play and printing.
