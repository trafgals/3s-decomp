# Third Strike Upper (3s') - Design Specification

> Source: <https://mikezsez.blogspot.com/2015/12/what-id-change-for-third-strike-upper-3s.html>
>
> Retrieved by `tools/fetch_design_spec.py`. This is the normative design
> document for the 3s' balance patch; the implementation status of each item
> is tracked in `docs/upper3s/spec-map.md`.
>
> **Note on numbers:** where this document disagrees with the decompiled retail
> data, the *intent* of the change governs. Concrete values are re-derived from
> the retail tables rather than transcribed, and the reasoning is recorded in
> `docs/upper3s/spec-map.md`.

---

[I almost wrote one of these for GGXXAC++R, but this one decided to get finished first.]

I **love **Street Fighter III: Third Strike (3s).  I like it better than all the other Street Fighter games combined, partially _because_ it doesn't feel like an SF2 game, which is the exact thing that a lot of people who dislike Third Strike don't like.  :^P

However, 3s has some problems - both with balance and system mechanics.  Most people who play it will readily admit this.  3s was never updated or fixed.  It has also never gotten a really "acceptable" console port, with no extra input lag and with all features intact, to the point where you could play said port and be ready to play it on an arcade setup with no adaptation.

So I figured, what the heck, if I ever got to do a 3s port - laugh it up, fuzzball - I'd like to make it 3s' (read as Third Strike Upper / Third Strike Dash, after SF2') and try to tackle tackle headbutt some of the problems the game has.

**The goal of 3s' is to change the way the game is played AS LITTLE AS POSSIBLE while still fixing problems.**  So you won't find any "remove Yun" type things, and I will try to justify each suggested change.  However, that _does _mean that things which are central to the current metagame _will_ change, if those things are sufficiently unbalanced to warrant attention.  My aim as a designer is to only nerf super dominant tactics a tiny bit to make them less dominant, rather than removing them entirely or drastically changing them.  I much prefer to fix balance by making under-used supers useful, making the low-tier characters better, and fixing bugs or inconsistencies...so there is a lot of those things, instead.

Format:

**Problem** - Solution. _Explanation._

OK, here goes...

###
System Mechanics:

**Whiffing normal attacks for meter **- Each successive whiffed normal now scales the whiffed-normal metergain rate by half (100%, 50%, 25%, 12.5%, etc).  Whiffed specials are not changed. The scaling is reset whenever a move touches the opponent on hit/block/parry, or your character gets hit, blocks, or parries. Blocked projectiles do not reset this scaling for either character, projectiles that hit do reset it, and projectiles that are parried reset it for the character that threw the projectile only. _Whiffing normals at full screen to build large chunks of meter is a main part of 3s.  Completely removing the metergain on whiffed normals solves the problem, but fundamentally changes the game. This solution reduces the usefulness of whiffing multiple normals, but still allows one or two to build up a bar when you are close to full and allows zoning to be at least as useful as before, if not a teeny bit more useful._

_
_

**Parries - **+2f on the cooldown period after attempting a parry, before another parry is allowed.  During the cooldown period for a specific parry, if you are hit with an attack that is parried the same way as the parry you just missed, you suffer crouching damage bonus.  For example, if you attempt a Forward(6) parry and miss, and are then hit by an attack that can be parried with Forward(6) during the cooldown period, you take 25% extra damage.  However, if you attempt a Forward(6) parry and miss, and are hit by an attack that must be parried Down(2), you will not take any extra damage. Being thrown out of parry cooldown does not grant extra damage. _This change is intended to penalize people for mis-timing parries, without penalizing them extra for guessing wrong.  If you mis-time a parry but it was the wrong parry, you take regular damage.  If you mis-time a parry but it was the _correct _parry, you take extra damage.  And, of course, you can still block at any time, even during parry cooldown. **Successful parries are unchanged.**_

_
_

**"Guess Parries"** - Parries input during any wakeup animation, recovery roll, or air land-on-feet hitstun are only active for half as long as a regular parry, with the same cooldown period. If you are thrown during the parry window or cooldown from one of these parries, you cannot tech the throw. Parries input during throw-tech animations are not changed. _Guess parries are very powerful because you sacrifice nothing by attempting one.  This allows you to still try guess-parrying, you're just required to have better timing and commit slightly more because you're open to throws.  They are still easier than red parries, but this makes varying your okizeme tactics or timing more effective against people who guess-parry. And combined with the damage bonus on missed parries, it makes guess-parrying _much _riskier._

_
_

**"Charge Partitioning"** - As soon as any charged direction (Back[4] or Down[2]) reaches 1/3 charge, the charge partition timer will be reset to maximum. _Charge partitioning is a nice advanced technique that is present only in Third Strike, however the amount of time you have to partition your charge is RANDOM depending on where that hidden timer happens to be when you start charging. This is not so great, and is easily fixed. This change ensures charge partitioning is always as easy as possible._

_
_

**Left-Right Unblockables - **Left-right unblockables are not fixed, but any time a character is hit during block animation while they are facing the opposite direction from the hit, the damage scaling is reduced to 50% for that hit and the followup combo. _Unblockables are sort of a core part of 3s at this point, so removing them is undesired because it would drastically alter some characters, even if removing unblockables is preferable from a design standpoint.  This change simply reduces the effectiveness of unblockables a bit, while still allowing them to exist._

_
_

###
Characters:

**Chun-Li:**

- Houyoku-sen (SAII) now has a maximum of 1 stock instead of 2; the bar is now the maximum length possible, the same as Shin-Shoryuken.  _Really now, she needs this.  The super still does a decent amount of stun, too._

- Cancel window on cr.Fwd (c.MK) fixed to not allow super-late cancels.  _She needs this too, perhaps these two changes together will encourage more diverse super-art choice.  She still has all her other amazing normals, kara-throw, EK SBK and Lightning Legs, etc._

- Crouching Jab (2LP) startup increased from 2f to 3f.  _She's one of the only characters in the game with a 2f normal, and she's short, and it's crouching which means it's usable as a crouch-tech.  Throws are still universal 2f punishes.  She does NOT need this._

- Tensei Ranka (SAIII) startup hitbox reaches down to the floor so it can't be crouched; changed to pull in and downward so all hits connect vs airborne opponents more often; advantage on knockdown increased enough to allow crossup attempts afterward; maximum of 2 stocks available; bar is a bit longer, the same length as Shippu.  _Trying to make this super actually useful, let's make it her setup super._

- Middle hits of Houyoku-sen fixed to not un-combo occasionally vs crouching Makoto, etc.  _Bugfix._

- **Chun-Li now goes through the mandatory one-standing-frame after wakeup**, like every other character.  _Bugfix._

- **Increase the height of her crouching head hurtbox (vulnerable box) by 2-3 pixels upward and 3-4 pixels backward in all applicable animations:** crouching neutral, crouch-block, non-low-profile crouching attacks, etc.  _Bugfix!  She's the shortest crouching character by quite a bit, and lots of things that should hit crouchers, like Remy's close Strong, Oro's standing Forward, or Dudley's EX Machine Gun Blow, miss on only Chun-Li._

**Ken:**

- Shippu Jinrai Kyaku (SAIII) now has a maximum of 2 stocks instead of 3; damage reduced by 15%.  _Even if Shippu didn't do any damage it would still be the most useful super in the game because it converts any normal into a setup, and 15% puts it just below Spinning Beat.  It's only _helped _by worse guess-parries, too._

- Jab DP (623LP) damage reduced from 15 to 11.5.  _He can still do it twice, and doing two is still more damage than doing a Strong DP (623MP) but is not twice as much._

- Grounded opponent knockback on 2nd hit of Target Combo Strong->Fierce decreased slightly, to help DPs miss less often afterward / in the corner.  _Legitimate bugfix, because this sucks._

- Fix the crash when killing Makoto with a neutral throw.  _Depending on which version of 3s we're starting with, this could still be present.  :^P_

**Yun:**

- Genei Jin (SAIII) no longer grants all his normals increased priority.  _Yes, Third Strike has actual priority, and yes, Genei Jin makes ALL of Yun's normals count as Super hits, which crush all normals and specials during trades.  You cannot out-poke him during it, and his Jab will beat all special moves and trade with supers.  Let's fix that!_

- Genei-Jin bar length increased, now same length as Sourai Rengeki.  Duration of timer for the super is not changed.  _Note that the damage scaling during Genei Jin juggles has not been changed._

- Sourai Rengeki (SAII) now a maximum of 2 stocks instead of 3.

- Upkicks (623K) hitbox adjusted to reach further downward, so it misses standing/crouching opponents less often.  _Bugfix._

**Makoto:**

- Stun damage of Abare Tosanami (SAII) reduced by 50%.  _The super itself doesn't do that much stun, it's the other moves that lead to 100%, but we'd like to affect her gameplay as little as possible otherwise, and this lets us not have to adjust the stun on Fukiage as much as we'd have to if we didn't mess with the stun on SAII._

- Stun damage of L/M/H Fukiage reduced from 29/31/31 to 27/28/31. _Fixes 100% stun combo, along with above change._

- Tanden Renki (SAIII) can be manually deactivated early by inputting QCT+K (236K) while the super art is active.  The deactivation counts as a special move, and uses the same animation as her taunt after a Hayate, but it is cancellable at any time into attacks (not blocking or movement). _This super increases her damage but prevents her from blocking while it is active.  By allowing her to deactivate it and return to normal, it's buffed quite a bit because she doesn't have to suffer through 3-4 knockdowns with guess-parries.  And being able to use the deactivation to cancel normals on hit/block would be interesting._

- Karakusa startup animation will no longer automatically throw-tech if Makoto is thrown out of it.  _Bugfix, and even as a Makoto player I think this is dumb.  :^P_

- Increase vacuum effect and hitstun time on the first hit of Seichusen Godanzuki (SAI), to help it fully connect from all distances or as a juggle.  _Bugfix._

- Fix bug where EX Oroshi (214PP) will not knock down when it connects with an opponent's limb hurtbox, as opposed to a body hurtbox.  _Bugfix.  It's cool but unpredictable, and she'd rather have the knockdown._

- Fix bug where one frame of Hayate's (236[P]) startup/hold animation has a get-thrown-box that is too small compared to its' pushbox, resulting in it not being throwable even by many super throws.  _Bugfix, verified the bug by checking hitboxes.  Thongboy Bebop will thank me, we still remember that match.  :^)_

- Remove invincibility on Makoto's recovery animation after a successful Karakusa (63214K); no change to invincible recovery animation on neutral throw (headbutt).  _Not necessary on the Karakusa, but changing this on her neutral throw would possibly leave her at a disadvantage, so even though it's a bug it's sort of the only real solution._

- No change on EX Oroshi xx Abare Tosanami comboing on Hugo.  _With the changes to her stun, this is more for style than anything now._

**Dudley:**

- Reduce stun damage on Rocket Uppercut (SAI).

- Increase stun damage on Rolling Thunder (SAII).  _Try to differentiate them somewhat._

- Fix Rocket Uppercut's "character point" position to not knock opponent backwards as often in juggles.  _Bugfix._

- Fix Corkscrew Blow to vacuum the opponent in on hit, so that it fully hits more often in juggle situations.  _Bugfix._

- Fix Rolling Thunder (SAII) to turn around if the player makes him dash again but he goes underneath the opponent.  _Bugfix._

- See if there is some way of making Cross Counter connect against invincible moves, rather than whiffing after taking the damage.  _Bugfix.  If Valentine can do it, Dudley can do it.  (^.^)_

- Dudley's pretty alright, actually.  (^.^)

**Urien:**

- Fix Tyrant Slaughter/Tyrant Punish (SAI) to fully connect for maximum damage vs all characters when crouching or in most juggle situations; increase damage by 25% to match Shin Shoryuken; limit to 1 stock instead of 2; increase bar length slightly.  _Bugfixes, more in line with what the intent of the super seems to be - pure damage.  Still can be linked after lots of things on the ground._

- That's most of Urien...unblockables and charge partitioning are dealt with in System Mechanics, and vs-Alex juggles are dealt with under Alex's section.

**Akuma:**

- Stun bar increased to "regular" size instead of "short" size.  _His health can be left as-is, but considering one stun is basically death with that amount of health, let's make that just a teensy bit more difficult to accomplish._

- Messatsu Gou Rasen (SAII) bar length is now shorter than SAI or SAIII.  _Earlier access to Raging Demon/KKZ at the cost of having a terrible super otherwise._

- KKZ (hidden super) motion changed from D,D,D+PPP to D,D+PP; damage reduced from 107 to 95.  _First hit is still unparryable, but for the only guaranteed reset in the game let's make it slightly less amazing._

- Towards+Strong is still invincible to throws.  _This is useful, keep it!_

**Yang:**

- Alter the trajectory of the divekick in Target Combo 3 (jump towards MK->DF+MK) so that it hits more often; alter the trajectory of bug Target Combo 3 (jump backward MK->DF+MK) so that it lands faster.  _Bugfixes._

- Fix bug where close standing Forward (MK) will sometimes not launch the opponent.  _Bugfix._

- Fix bug where Yun/Yang can end up behind Yang after being hit by Fierce Mantis Slashes (236HPx3) with a certain timing.  _Bugfix, probably Yang's pushbox._

- Actually, Yang's pretty okay too.  (^.^)  I'm fine leaving Sei'ei Enbu with the shortest bar in the game, because it is not anywhere near as dominant as Genei Jin, and costs him the use of EX slashes.

**Ryu:**

- Decrease opponent knockback from F+Strong (6MP) on hit, to make links after it more reliable.  _Bugfix._

- Increase forward movement on 2nd hit of Target Combo (far s.Fierce->s.RH), so that the s.RH will connect more reliably.  _Bugfix._

- Increase hitstun on far standing Forward, standing Roundhouse, close Fierce, crouching Fierce, crouching Short, and crouching Forward so they are not negative on hit_.  Bugfixes._

- Expand hitbox on LK Joudan Sokutou Geri (41236LK) downward so that it will not miss on some characters after close Fierce (HP) or crouching Fierce (2HP).  _Bugfix, because it misses on about half the cast for no real reason.  The alternative is cr.Fierce forcing standing hitstun, but then he can always combo to Hurricane Kick so that might be too much of a buff._

- Lower opponent's knockup height from Jab and Strong DPs (623LP/MP), and reduce knockup height on the middle hits of Shinkuu Hadoken (SAI) so that all hits of the super will connect after a DP.  _Bugfix._

- Ryu is strong.

**Oro:**

- Examine pushboxes on Jinchu Nobori ("chicken kick") to help the [s.Strong xx Chicken Kick]x2 combo work on every character.  _Bugfix._

- Increase hitstop and allowed juggle time on Oniyama ([2]8P), to make super cancels out of it actually useful.  _Bugfix-ish, gives him more reason to use the move._

- Add forward throw (Towards+LP+LK) that uses the animation from his back throw but mirrored to toss them forward; only tosses them about half as far as his back throw.  _Oro currently has no way to throw without switching sides, which means he has to give up the corner at all times._

- Fix neutral throw so that it cannot be escaped before 3-4 hits, the same as all other mashable grabs.  _Bugfix._

- Increase length of the Kishi Riki (SAI) bar to match Tengu Stone's bar; make Human Pillar Driver (63214P) still be a hit-grab during Kishin Riki, so that Oro can combo into it from things like s.Forward (s.MK); increase damage of the air regular Kishi Riki grab when combo'd into; allow Oro to deactivate the regular Kishi Riki by inputting QCT+P during the super, using a shortened version of the activation animation.  _Kishin Riki is his least useful super.  Human Pillar Driver change is a bugfix - you can still special cancel into it during Kishin Riki, but since it becomes a regular throw it won't combo._

**Elena:**

- Allow her to chain crouching Short (2LK) into itself.  _She has no confirmable lows, crouching Strong into super doesn't always combo, and she can't link supers off her command overheads like Dudley can, except in very special cases.  This gives her a confirmable low at least._

- Make startup frames of all versions of Mallet Smash unthrowable, like they look.  _Bugfix/buff, Mallet Smash has 7f of throwable startup - she's even throwable during the frame where she has already left the floor!_

- Allow cancels from the 2nd hit of standing Strong, not just the first hit.  _Small buff._

- Fix Brave Dance so it juggles properly vs airborne opponents outside the corner.  _Bugfix/buff._

- Speed up the cancel animation on Healing (SAIII), increase the rate of health recovery.  _Attempt to make it useful._

- She dances okay!  _Leaving her with 3 stocks for Spinning Beat (SAIII), also._

**Ibuki:**

- Health increased from 1035 to 1050, same as Twelve's health.  _Buff._

- Yami Shigure (SAIII) now has 2 stocks instead of 1.  _Buff._

- Allow Tsuiji Goe (623P, command jump) to be special- and super-cancelled much earlier in the animation, almost directly after takeoff; make Kunai cancelled from it face the correct direction if it hasn't passed over the opponent yet.  _Buff and bugfix._

- Fix the unique-animation standing Fierce in her Target Combos (B+Strong->Fierce and Jab->Strong(1)->Fierce) to never knock down, instead of sometimes resetting and sometimes knocking down.  _Bugfix: there are two simultaneous hitboxes and only one is marked as knockdown, so it depends on which one the game checks first.  After feedback from more than one Ibuki player, it seems "always reset" is preferable to "always knock down"._

- Remove the ability to throw-tech her Personal Action (taunt), and allow Ibuki to parry or attack in the air once the opponent recovers.  _Bugfix, and since it is currently VERY punishable on hit, might as well make it a semi-useful mixup option..._

**Necro:**

- Change Slam Dance (SAII) startup from 0+2 to 2+0, so the total startup time is unchanged but it can't be jumped on reaction after the superflash.  _It could use improvement._

- Fix pushbox on Electric Shock (623P), and add a slight vacuum effect to the middle hits of all versions so that all hits of it connect more reliably, since it is so unsafe.  _Bugfix._

- Add EX Electric Shock, possibly allows juggles afterward, certainly safer on block than the Fierce version.  _Buff, and strange that he hasn't got it to start with._

- There's not a lot else I can think of to buff for Necro that wouldn't immediately make him crazy good.  He has crouching Short xx Jab Tornado Hook to be safe on block and hitconfirm into super, and giving him a cancellable crouching Forward would be insane.

**Alex:**

- Change Hyper Bomb (SAI) startup from 0+1 to 1+0, so the total startup time is unchanged but it can't be jumped on reaction after the superflash.  _Make it slightly more useful.  POSSIBLY reduce the damage a tiny bit unless it is the Backdrop Bomb (back-turned version) to compensate._

- Increase forward movement on Boomerang Raid's middle hits, and increase the range on the final grab, to prevent the grab from whiffing after the rest of the super has connected at medium range.  _Bugfix._

- Make all versions of Air Knee Smash not air-parryable; EX version is still ground parryable.  _Bugfix. The L/M/H versions can't even hit grounded characters so they are supposed to be antiairs, but being parryable makes them all horrible.  Shootdown Backbreaker is not air-parryable, and this gives Alex standing Strong xx Air Knee Smash vs air parry-happy players._

- Increase the cancel window for his Target Combo (cr.Short->cr.Fwd) to be longer than 1f, so that it can be done as easily as any other Target Combo in the game.  _Bugfix._

- Allow juggles after Flash Chops that connect vs airborne opponents.  _Bugfix, and situational anyway._

- Allow crouching Strong (2MP) to be special- and super-cancelled on hit/block/parry.  _Buff, he sorely needs this._

- Allow crouching Forward (2MK) to be super-cancelled on hit/block/parry, including when used as part of his Target Combo._  This and the cr.Strong change will make him miles better._

- Fix Air Stampede ("stomp", [2]8K) not knocking down if it hits a limb-hurtbox rather than a body hurtbox.  _Bugfix._

- **Fix the position of the anchor point and pushbox for his air knockdown animation to allow juggles to work properly on him.**  _Bugfix and stealth nerf, he's pretty much the only character with this much of a problem and it gets him out of a lot of things for free.  Elena has a weird offset but it's just visual for her, the juggles still work._

- Fix position of hitboxes on j.D+Fierce (j.2HP) dive, to place them on his arms instead of his chest.  _Bugfix._

**Hugo:**

- Allow Monster Lariat (236+K) to be super-cancelled on hit, and allow supers to juggle after it.  _Megaton Press is his least-used super, this would provide a reason to pick it by giving you crouching Short xx Monster Lariat xx Megaton, whereas Hammer Mountain Frenzy would not fully connect._

- Decrease startup of EX Giant Palm Bomber (63214PP) from 22f to 17f, the same as the Jab version. _Give him ANY reason to use it!  :^P_

- Increase hitstun of crouching .Jab (2LP) and crouching Short (2LK) by 1-2f, to allow combos to EX Monster Lariat from farther away.  _It's pretty much his only confirm option, but is randomly blockable depending on distance._

- Add EX Shootdown Backbreaker, which allows juggles afterwards like his 2i version; distance can be controlled with LK+MK / MK+HK / LK+HK. _Improvement, his other EXs are pretty situational._

- Reduce startup of L/M/H Meat Squasher (360K) from 21/31/44 to 18/24/30; reduce recovery on whiff from 30/33/36 to 26/29/32.  _It may still not be useful, but at least it'll be less "never-ever-ever use it"._

- Fix the wrongly-offset hitboxes on frame 1 of his landing-from-air-hitstun.  _Bugfix._

**Q:**

- Allow crouching Jab, standing Short, and crouching Short to be special- and super-cancelled on hit/block/parry.

- Allow crouching Forward to be super-cancelled on hit/block/parry.  _These sound like a lot, but Ken (and now Elena) have way more options.  Q could use at _least _this._

- Allow all overhead/low rushpunches ([4]6P/K/[P]) to be super-cancelled on hit/block/parry.  _Make the riskier versions at least AS useful as the safe version._

- Decrease the startup of far standing Jab from 6f to 4f._  It's not a super amazing move and this will help some of his problem matchups, like Akuma, quite a bit._

- Extend the cancel window on B+RH (4HK) past the first active frame, so it can be cancelled from further away.  _Bugfix._

- Increase hitstun on crouching Short, crouching Forward, and standing Roundhouse so they are not negative on hit.  _Bugfix.  Standing Roundhouse is -10 on hit, for goodness' sake._

- Change opponent's movement when juggled with the middle hits of all High Speed Barrage ("slaps") versions, so that all of the hits will connect from more ranges and at more heights.  _Bugfix._

- Extend hitbox of first hit of EX High Speed Barrage downward so that it more consistently connects against crouching characters.

- Add EX (PP) version of Total Destruction (SAIII) which immediately does the grab part upon activation, as with Oro's Kishi Riki.  _Let's imagine we want this super to be useful._

- Do not allow the punch version of Total Destruction to trade hits.  _Bugfix, which occasionally causes Q to die._

- Allow Q to deactivate Total Destruction by entering QCT+P while the super is active.  _It might not be used, but most other timer-based things got it._

- Decrease health bonus for having 1-3 Personal Actions (taunts) from 1200 health base/1400/1705/2050 to 1200 health base/1400/1600/1800.  _To compensate for him being a decent character, need to slightly reduce his 200% health bonus for 3 taunts._

**Remy:**

- Stun bar increased to "regular" size instead of "short" size.  _I assume this was because he's supposed to be a zoner, but t_here's no reason for him to be a higher-risk character this way with how bad zoning ended up being in 3s.

- Increase damage of crouching Strong by 6pt to almost match Ryu's.  _It seriously does the wrong amount of damage._

- Fix the earlier hitboxes on close Fierce to knock down vs airborne opponents like the later hitboxes do, rather than have them land standing; allow juggles afterward.  _Bugfix, give him a _consistent _ability to use this move as antiair._

- Increase hitstun of far Forward, far Roundhouse, crouching Forward, and Towards+Forward (6MK) so that they are not negative on hit.  _Being punishable by Houyoku-sen after hitting Chun-Li with all these moves is seriously wrong._

- Decrease grounded opponent knockback on standing Forward (MK), enlarge hitboxes on 2nd hit of Target Combo (standing Forward->RH) so that it connects on everyone standing and most people crouching.  _Bugfix, it completely whiffs against Yun/Yang/etc at point blank when standing...how useless..._

- Allow chaining from crouching Jab->crouching Short and crouching Short->crouching Jab.  _Give him _some _kind of low confirm, since he has to be charging already in order to use it._

- Decrease total length of Personal Action (taunt) from 131f to ~80f.  _Still situational, but no longer entirely impractical.  Makoto's is 83f or so._

- Increase meter gain from all Light of Virtues on startup/hit/block from 0/1/1 to 1/3/2.  _Give his projectile meter for executing it, and more meter when it connects, to help his zoning be actually useful._

- Allow all strengths of Rising Rage Flash to be super-cancelled on hit/block/parry; allow juggles after all versions of Rising Rage Flash.  _Make the risky reversal more useful, allow juggles in situations where it hits vs high airborne opponents._

- Increase hit advantage of Short/Forward Cold Blue Kicks (63214K) so they are not negative on hit from up close; make Forward version +2 so that supers can be linked off it at point-blank.  _Bugfix, and buffs._

- Remove superflash on activation of Blue Nocturne (SAIII) pose, only show the superflash after it successfully counters something; still always spends the meter but not on activation, only on successful counter/recovery from pose/being thrown; increase bar length by a bit; make it properly juggle/knock down vs air.  _Bugfixes, let's make it a real freakin' counter super._

**Twel(e)ve:**

- Health increased from 1050 to 1120, the same as Sean/Remy/Necro/etc.

- Increase hitstun on standing Strong, far Strong, standing Fierce, crouching Fierce, standing Short, standing Forward, standing Roundhouse, crouching Forward, and Back+Forward (4MK) so they are not negative on hit.  _Really now..._

- Make crouching Strong and standing Forward only parryable high, not both ways; make crouching Fierce and Back+Forward (4MK) only parryable low, not both ways.  _Bugfixes/buffs, depending on how you look at the moves._

- Activate the earlier hitboxes in Back+Forward (4MK) that exist but cannot connect.  _Bugfix.  The hit spark is positioned as if the earlier low hitboxes are supposed to connect, but they can't for some reason._

- Allow far Strong, standing Strong, and crouching Strong to be special- and super-cancelled on hit/block/parry.

- Allow crouching Strong to be superjump-cancelled on hit/block, like standing Forward, so that he has more options into X.F.L.A.T.

- Allow crouching Forward to be super-cancelled on hit/block/parry.  _Why is he SO limited?!_

- Increase hit and block advantage of Jab A.X.E. (214LP) so that it is at most -1 or -2 on block.  _He has nothing even close to safe..._

- Allow all air normals to be special-cancelled on hit/block into A.X.E. (j.214P) or D.R.A. (j.214K) and super-cancelled into X.F.L.A.T (SAII).  _Air mobility is his "thing", why not let it go a bit further..._

- Change opponent trajectory when juggled with X.N.D.L. to drag them down so that more hits of it will make contact if they are hit higher up.  _Bugfix._

- Remove low-damage air-to-air version of X.F.L.A.T., have it always do full damage even vs air; increase number of stocks to 2.  _Justifiable buff, the air-to-air version is _terrible_._

- During X.C.O.P.Y., allow Twelve to do EX moves, and maybe even the opponent's chosen super, by spending chunks of the remaining timer; reduce X.C.O.P.Y. startup from 12f to 1f so that comboing into and out of it is easier; when hit during X.C.O.P.Y.'s recovery, instead of taking increased damage, Twelve now takes 66% damage.

**Sean:**

- Increase hitstun of close and far Fierce, crouching Fierce, standing Forward, far Roundhouse, crouching Forward, and Towards+Fierce (6HP) to not be negative on hit.  _Bugfixes._

- Allow Sean Roll (214P) to be super-cancelled during the middle.  _Pretty useless move right now._

- Decrease the startup of L/M/H Ryuubi Kyaku (236K) from all being 25f to 17/19/21f; make Roundhouse version knock down.  _Currently they all have the same startup, which means parrying them is no different, and they basically all have the same use._

- Increase hit advantage of L/M versions of Ryuubi Kyaku (236K), to allow links to supers after hitting. _Currently they are all negative on hit._

- Decrease the startup of all Tackles so that cr.Fwd xx Jab Tackle combos, standing/crouching Fierce xx Strong Tackle combos, and Fierce Tackle is 1f away from comboing in that situation.

- Allow Fierce Tackle to be faked/cancelled by tapping K during startup.

- Make Fierce and EX Tackles un-ground-parryable.

- Change EX Tackle (236PP~P) so that the hitbox is active from 8f-end and it will automatically grab whenever it reaches the opponent; allow EX Tackle to be super-cancelled on the last hit.  _Tackle is also pretty crap, it's very unique so how about we make it a move he wants to choose, instead._

- Reduce startup of L/M/H/EX Dragon Smash (623P) from 5/5/9/8f to 3/4/5/3f.  _Somewhere between Ken and Ryu._

- Fix Hyper Tornado (SAIII) so that it juggles properly vs airborne opponents.  _If Goodnight Sugah in MvC2 can do it, surely Hyper Tornado can._

- Decrease recovery on Hadou Burst from 61f to 51f.  _Still unsafe, but a bit more setup time on hit._

- I would have suggested replacing his Fierce/EX DP animation with the extra-hit dunk he had in Second Impact, but I don't know how possible that is to actually do.

###
Additional Bugfixes:

**Make All Successful Throws Invincible for the Entire Animation **- It can cause bad situations or bugs when characters are hit out of a successful throw animation, like the Remy/Urien glitch or the Q throw bug.  _It's also good game design._

**Fix Bug Where Characters Are Allowed To Move During The Opponent's Superflash** - A rare bug that occurs if you interact with a projectile on the same frame your opponent's superflash starts. _I've seen it happen in real matches, so it deserves to be fixed._

_
_

**Allow Universal Overheads (MP+MK) and Personal Actions (HP+HK) to be Done While Holding Away or Towards ** - Bugfix, it only works for throws. _There's really no reason for this not to be possible, and it might open up some new kara-overhead options like Ryu Towards+Fierce._

_
_
**Fix Bug Where Some Characters Can Attack Immediately After Teching Air Throws** - Makoto, etc.  _Self-explanatory._

**
**

**Fix Rare Move Interaction Bugs** - X.F.L.A.T. vs EX Yagyou Dama / Aegis Reflector / Tengu Stone, Critical Combo Attack / Deadly Double Combination vs Magnetic Storm, etc.  _Fix soft-lock conditions or other serious problems.  Not fixing being able to OTG Dudley or Hugo with certain supers after being thrown, as all of those are rollable._

_
_
**Characters Passing Through Each Other** - NOT FIXABLE - Interactions such as Makoto's EX Hayate vs Chun Li's dash, or Chun Li's dash vs Yun's dash / lunge punch...are unfortunately not able to be fixed without a major rewrite of the collision system to make it "continuous collision".  As this would change the game significantly, I would rather leave them as they are.  _At least it's very limited circumstances..._

_
_

**Move Motions - **If I had my way, all special moves that are half-circle motions (41236 or 63214) would be replaced with quarter-circle motions, either on the same button if there is no overlapping special move or on another button if there is an overlapping special move.  Akuma's red fireball might stay a half-circle, but that's all.  _There's no reason Karakusa/Machine Gun Blow/Ducking/Cross Counter/Capture&Deadly Blow/etc should require half-circle motions when there is no corresponding quarter-circle motion to distinguish them from.  Inputting a half-circle would still work, since it contains the quarter-circle._

_
_
_--_

_
_
This was pretty in-depth, but I take this kind of stuff in game design seriously.  Even if there is no chance in hell of ever actually making this, I treated it as if I were.

Special thanks to Alex Sanchez for proofreading.
