# Akashi

## What is Akashi?

**Akashi** is a next-generation video editor. 

You can edit your videos only by programs without being bothered with complex GUI.

## Why Akashi?

### The development style of existing video editors

You may have used video editors for making your own videos. If so, what is your impression about them? In my impression, they are just terrible. The characteristics of them are summed up in three keywords: **complex**, **hard to memorize**, and **hard to customize**.

#### 1. Complex

First off, I must say that existing video editors are too visually complex. Users are overwhelmed by numerous complex widgets, including a pile of panes that occupy the entire screen, a gorgeous timeline sequence enough to baffle and perplex beginners completely, and so on.

GUI is faced with a fundamental limitation; *as the size of the features increases, the size of widgets increases on a linear scale*. I call this limitation **the curse of GUI**. **The curse of GUI** is inevitable because of another limitation; *all the features supported by GUI need to be visualized*. Visual complexity in existing video editors is also inevitable because of **the curse of GUI**.

To implement too many features, video editors often have a menu of a menu. In this case, to do a certain thing A, you must recursively trace the menu tree to find the leaf A in such a way that you open the menu M1, then select the sub menu M2, then select the sub menu M3, and so forth. 

Video editors have a widget called inspector which shows the states of an object, and offers the interface to change its states. The objects include layers, assets, video information, or anything related to video editing, and all of them have dozens of states. So, inspector is a nest of complexity because the states of objects need to be visualized for them to be accessed.

These examples are a product of **the curse of GUI**, and visual complexity in them cannot be removed as long as video editors rely on GUI.

#### 2. Hard to memorize

On the other hand, **the curse of GUI** forces users to remember precisely where the widgets are, and memorize every single one of the steps of the each widget's usage.

To escape from **the curse of GUI**, hotkeys tend to be abused in video editors. With hotkeys, users are freed from visual complexity, but the cost is not low. On a relatively small scale, hotkeys solution works, but on a larger scale where hundreds of features are supported, the cost of memorization becomes not negligible.

User's memory is not abundant, so practically speaking, the number of effective hotkeys is not so large. Either way, users cannot escape memorization.    

#### 3. Hard to customize

Basically, existing video editors offer users very limited ways of customization. There are few ways of abstraction, and in most cases all you can do is to plug-in the extension to your video editors. Scripting features added afterwards are only applicable to the limited areas of the editing procedure.

### The development style of GUI applications

As an aside, let's take a look at the relationship between videos and GUI applications(or simply apps). As a matter of fact, videos and apps are basically the same; both are a real-time graphical application which holds a main loop displaying images continuously. You could argue that both are different in that videos cannot handle side effects like user inputs, and apps can handle them. But in reality, videos can handle side effects if the engine implements so, and that is utterly possible and exists in reality as a form of [interactive video](https://en.wikipedia.org/wiki/Interactive_video). After all, it's a matter of definition. So please, if you are a skeptic, accept the assumption that there is a multimedia which holds a main loop above mentioned, and the multimedia encompasses videos and apps, and in that sense, both can be treated the same way. 

Then, even with the fact videos and apps are the same, why does not exist a video editor which offers a way of developing videos like apps? The development style of app is clearly way more superior to that of existing video editors. The advantages are the opposite of the disadvantages of video editors that I mentioned earlier: **simple**, **easy to learn**, and **easy to customize**.

#### 1. Simple

Freed from **the curse of GUI**, no matter how many features you add, the screen will never get more complicated. Especially if you are using a simple editor like Vim, you basically only need a single pane.

#### 2. Easy to Learn

Freed from **the curse of GUI**, there is no need to memorize things like spatial arrangement of features and their usage and a huge number of hotkeys. All you need to do is to learn the concepts around application development.

#### 3. Easy to Customize

Undoubtedly, customization includes a plug-in. As I mentioned earlier, the only way of customization provided by existing video editors is a plug-in. In GUI context, you can say that to increase the number of widgets is customization.  In application development, a plug-in is possible in a way of adding libraries or extensions.

Customization, however, is not just about proliferation. It does include abstraction. Abstraction needs to be exercised by texts, so that is not possible for GUI. But in application development where apps are developed by programs, which are composed of texts,  customization by abstraction is possible.

Concretely,  you can think of a procedure of a procedure, and combine them freely. You can polish and sophisticate your design with the support of the power of text. You can share your thoughts and ideas with others through codes. More importantly, if you write once, you do not need to work because your computer will do the work for you. Of course, the power of such automation can be easily shared with others.

### The development style of Akashi

If we can incorporate the application development style with video editing, what will happen? I can imagine how wonderful and glorious that such future will be.

After all, the first and top priority of **Akashi** is the maximization of user experience. Freeing people from the pain of video development is the goal of **Akashi**, and the greatest value we can provide to you.

## Getting Started

TBD

## Contributing

TBD

