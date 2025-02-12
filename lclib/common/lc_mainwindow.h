#pragma once

#include "lc_basewindow.h"
#include "lc_array.h"
#include "lc_commands.h"
#include "lc_model.h"

/*** LPub3D Mod - status and logging ***/
#include <QToolBar>
#include <QStatusBar>
#include <name.h>
/*** LPub3D Mod end ***/

class View;
class lcPartSelectionWidget;
class PiecePreview;
class lcQGLWidget;
class lcQPartsTree;
class lcQColorList;
class lcQPropertiesTree;
class lcTimelineWidget;
#ifdef QT_NO_PRINTER
class QPrinter;
#endif

#define LC_MAX_RECENT_FILES 4

struct lcSearchOptions
{
	bool SearchValid;
	bool MatchInfo;
	bool MatchColor;
	bool MatchName;
	PieceInfo* Info;
	int ColorIndex;
	char Name[256];
};

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
typedef QTabWidget lcTabWidget;
#else
class lcTabWidget : public QTabWidget
{
public:
	QTabBar* tabBar()
	{
		return QTabWidget::tabBar();
	}
};
#endif

class lcModelTabWidget : public QWidget
{
	Q_OBJECT

public:
	lcModelTabWidget(lcModel* Model)
	{
		mModel = Model;
		mActiveView = nullptr;
	}

	void ResetLayout();
	void Clear();

	QWidget* GetAnyViewWidget()
	{
		QWidget* Widget = layout()->itemAt(0)->widget();

		while (Widget->metaObject() == &QSplitter::staticMetaObject)
			Widget = ((QSplitter*)Widget)->widget(0);

		return Widget;
	}

	View* GetActiveView() const
	{
		return mActiveView;
	}

	void SetActiveView(View* ActiveView)
	{
		mActiveView = ActiveView;
	}

	void AddView(View* View)
	{
		mViews.Add(View);
	}

	void RemoveView(View* View)
	{
		if (View == mActiveView)
			mActiveView = nullptr;

		mViews.Remove(View);
	}

	lcModel* GetModel() const
	{
		return mModel;
	}

	void SetModel(lcModel* Model)
	{
		mModel = Model;
	}

	const lcArray<View*>* GetViews() const
	{
		return &mViews;
	}

protected:
	lcModel* mModel;
	View* mActiveView;
	lcArray<View*> mViews;
};

class lcMainWindow : public QMainWindow
{
	Q_OBJECT

public:
/*** LPub3D Mod - define lcMainWindow parent ***/
	lcMainWindow(QMainWindow *parent = 0);
/*** LPub3D Mod end ***/
	~lcMainWindow();

	void CreateWidgets();

	lcTool GetTool() const
	{
		return mTool;
	}

	lcTransformType GetTransformType() const
	{
		return mTransformType;
	}

/*** LPub3D Mod - coord format ***/
    lcRotateStepCoordType GetRotateStepCoordType() const
    {
        return mRotateStepCoordType;
    }
/*** LPub3D Mod end ***/

	bool GetAddKeys() const
	{
		return mAddKeys;
	}

	float GetMoveXYSnap() const
	{
		const float SnapXYTable[] = { 0.0f, 1.0f, 5.0f, 8.0f, 10.0f, 20.0f, 40.0f, 60.0f, 80.0f, 160.0f };
		return mMoveSnapEnabled ? SnapXYTable[mMoveXYSnapIndex] : 0.0f;
	}

	float GetMoveZSnap() const
	{
		const float SnapZTable[] = { 0.0f, 1.0f, 5.0f, 8.0f, 10.0f, 20.0f, 24.0f, 48.0f, 96.0f, 192.0f };
		return mMoveSnapEnabled ? SnapZTable[mMoveZSnapIndex] : 0.0f;
	}

	float GetAngleSnap() const
	{
		const float AngleTable[] = { 0.0f, 1.0f, 5.0f, 15.0f, 22.5f, 30.0f, 45.0f, 60.0f, 90.0f, 180.0f };
		return mAngleSnapEnabled ? AngleTable[mAngleSnapIndex] : 0.0f;
	}

	QString GetMoveXYSnapText() const
	{
		QString SnapXYText[] = { tr("0"), tr("1/20S"), tr("1/4S"), tr("1F"), tr("1/2S"), tr("1S"), tr("2S"), tr("3S"), tr("4S"), tr("8S") };
		return mMoveSnapEnabled ? SnapXYText[mMoveXYSnapIndex] : tr("None");
	}

	QString GetMoveZSnapText() const
	{
		QString SnapZText[] = { tr("0"), tr("1/20S"), tr("1/4S"), tr("1F"), tr("1/2S"), tr("1S"), tr("1B"), tr("2B"), tr("4B"), tr("8B") };
		return mMoveSnapEnabled ? SnapZText[mMoveZSnapIndex] : tr("None");
	}

	QString GetAngleSnapText() const
	{
		return mAngleSnapEnabled ? QString::number(GetAngleSnap()) : tr("None");
	}

	bool GetRelativeTransform() const
	{
		return mRelativeTransform;
	}

	lcSelectionMode GetSelectionMode() const
	{
		return mSelectionMode;
	}

	PieceInfo* GetCurrentPieceInfo() const
	{
		return mCurrentPieceInfo;
	}

	View* GetActiveView() const
	{
		lcModelTabWidget* CurrentTab = mModelTabWidget ? (lcModelTabWidget*)mModelTabWidget->currentWidget() : nullptr;
		return CurrentTab ? CurrentTab->GetActiveView() : nullptr;
	}

	lcModel* GetActiveModel() const;

	lcModel* GetCurrentTabModel() const
	{
		lcModelTabWidget* CurrentTab = (lcModelTabWidget*)mModelTabWidget->currentWidget();
		return CurrentTab ? CurrentTab->GetModel() : nullptr;
	}

	const lcArray<View*>* GetViewsForModel(lcModel* Model) const
	{
		lcModelTabWidget* TabWidget = GetTabWidgetForModel(Model);
		return TabWidget ? TabWidget->GetViews() : nullptr;
	}

	lcModelTabWidget* GetTabForView(View* View) const
	{
		for (int TabIdx = 0; TabIdx < mModelTabWidget->count(); TabIdx++)
		{
			lcModelTabWidget* TabWidget = (lcModelTabWidget*)mModelTabWidget->widget(TabIdx);

			int ViewIndex = TabWidget->GetViews()->FindIndex(View);
			if (ViewIndex != -1)
				return TabWidget;
		}

		return nullptr;
	}

	lcPartSelectionWidget* GetPartSelectionWidget() const
	{
		return mPartSelectionWidget;
	}

	QMenu* GetToolsMenu() const
	{
		return mToolsMenu;
	}

	QMenu* GetViewpointMenu() const
	{
		return mViewpointMenu;
	}

	QMenu* GetCameraMenu() const
	{
		return mCameraMenu;
	}

	QMenu* GetProjectionMenu() const
	{
		return mProjectionMenu;
	}

	QMenu* GetShadingMenu() const
	{
		return mShadingMenu;
	}

	QByteArray GetTabLayout();
	void RestoreTabLayout(const QByteArray& TabLayout);
	void RemoveAllModelTabs();
	void CloseCurrentModelTab();
	void SetCurrentModelTab(lcModel* Model);
	void ResetCameras();
	void AddView(View* View);
	void RemoveView(View* View);
	void SetActiveView(View* ActiveView);
/***	void UpdateAllViews();                 // LPub3D Mod - moved to public slots ***/
	void SetTool(lcTool Tool);
	void SetTransformType(lcTransformType TransformType);
    void SetRotateStepCoordType(lcRotateStepCoordType RotateStepCoordType);

	void SetColorIndex(int ColorIndex);
	void SetMoveSnapEnabled(bool Enabled);
	void SetAngleSnapEnabled(bool Enabled);
	void SetMoveXYSnapIndex(int Index);
	void SetMoveZSnapIndex(int Index);
	void SetAngleSnapIndex(int Index);
	void SetRelativeTransform(bool RelativeTransform);
	void SetCurrentPieceInfo(PieceInfo* Info);
	void SetShadingMode(lcShadingMode ShadingMode);
	void SetSelectionMode(lcSelectionMode SelectionMode);

/***	void NewProject();                     // LPub3D Mod - moved to public slots ***/
	bool OpenProject(const QString& FileName);
	void MergeProject();
	void ImportLDD();
	void ImportInventory();
	bool SaveProject(const QString& FileName);
	bool SaveProjectIfModified();
	bool SetModelFromFocus();
	void SetModelFromSelection();
	void HandleCommand(lcCommandId CommandId);

	void AddRecentFile(const QString& FileName);
	void RemoveRecentFile(int FileIndex);

	void SplitHorizontal();
	void SplitVertical();
	void RemoveActiveView();
	void ResetViews();

	void TogglePrintPreview();
	void ToggleFullScreen();

	void UpdateSelectedObjects(bool SelectionChanged);
	void UpdateTimeline(bool Clear, bool UpdateItems);
	void UpdatePaste(bool Enabled);
	void UpdateCurrentStep();
	void SetAddKeys(bool AddKeys);
	void UpdateLockSnap();
	void UpdateSnap();
	void UpdateColor();
	void UpdateUndoRedo(const QString& UndoText, const QString& RedoText);
	void UpdateCurrentCamera(int CameraIndex);
	void UpdatePerspective();
	void UpdateCameraMenu();
	void UpdateShadingMode();
	void UpdateSelectionMode();
	void UpdateModels();
	void UpdateCategories();
	void UpdateTitle();
	void UpdateModified(bool Modified);
	void UpdateRecentFiles();
	void UpdateShortcuts();
/*** LPub3D Mod - Timeline part icons ***/
    bool IsLPub3DSubModel(QString &Piece);
    const QString GetPliIconsPath(QString &key);
    const QString GetFadeStepsColor();
/*** LPub3D Mod end ***/
/*** LPub3D Mod - Rotate Angles ***/
	void GetRotStepMetaAngles();
/*** LPub3D Mod end ***/
/*** LPub3D Mod - parse and set rotstep line on model file load ***/
	void ParseAndSetRotStep(QTextStream& LineStream);
/*** LPub3D Mod end ***/
/*** LPub3D Mod - rotstep transform ***/
    lcVector3 GetRotStepTransformAmount();
/*** LPub3D Mod end ***/
	lcVector3 GetTransformAmount();

	QString mRecentFiles[LC_MAX_RECENT_FILES];
	int mColorIndex;
	lcSearchOptions mSearchOptions;
	QAction* mActions[LC_NUM_COMMANDS];
/*** LPub3D Mod - relocate toolbar ***/
	QToolBar* mToolsToolBar;       //moved from protected:
/*** LPub3D Mod end ***/
/*** LPub3D Mod - Timeline part icons ***/
    bool mSubmodelIconsLoaded;
/*** LPub3D Mod end ***/

/*** LPub3D Mod - status bar ***/
	QStatusBar* mLCStatusBar;
/*** LPub3D Mod end ***/

public slots:
	void ProjectFileChanged(const QString& Path);
/*** LPub3D Mod - relocate update and new project ***/
	void NewProject();               // move from public:
	void UpdateAllViews();           // moved from public:
/*** LPub3D Mod end ***/

/*** LPub3D Mod - halt, disable, enable viewer ***/
	void Halt3DViewer(bool b);
    void Enable3DActions();
    void Disable3DActions();
/*** LPub3D Mod end ***/

/*** LPub3D Mod - rotate step objects ***/
        void SetStepRotStepMeta(lcCommandId CommandId);
/*** LPub3D Mod end ***/

/*** LPub3D Mod - rotate step signals ***/
signals:
   lcVector3 GetRotStepMeta();   // used to capture step meta angles
   void SetRotStepMeta();
   void SetRotStepAngleX(float, bool=false);
   void SetRotStepAngleY(float, bool=false);
   void SetRotStepAngleZ(float, bool=false);
   void SetRotStepTransform(QString&, bool=false);
/*** LPub3D Mod end ***/

protected slots:
	void UpdateGamepads();
	void ModelTabContextMenuRequested(const QPoint& Point);
	void ModelTabCloseOtherTabs();
	void ModelTabResetViews();
	void ModelTabClosed(int Index);
	void ModelTabChanged(int Index);
	void ClipboardChanged();
	void ActionTriggered();
	void ColorChanged(int ColorIndex);
	void Print(QPrinter* Printer);

protected:
	void closeEvent(QCloseEvent *event);
	void dragEnterEvent(QDragEnterEvent* Event);
	void dropEvent(QDropEvent* Event);
	QMenu* createPopupMenu();

	void CreateActions();
	void CreateMenus();
	void CreateToolBars();
	void CreateStatusBar();
	void SplitView(Qt::Orientation Orientation);
	void ShowSearchDialog();
	void ShowUpdatesDialog();
	void ShowAboutDialog();
	void ShowHTMLDialog();
	void ShowRenderDialog();
	void ShowPrintDialog();

	lcModelTabWidget* GetTabWidgetForModel(lcModel* Model) const
	{
		for (int TabIdx = 0; TabIdx < mModelTabWidget->count(); TabIdx++)
		{
			lcModelTabWidget* TabWidget = (lcModelTabWidget*)mModelTabWidget->widget(TabIdx);

			if (TabWidget->GetModel() == Model)
				return TabWidget;
		}

		return nullptr;
	}

	QTimer mGamepadTimer;
	QDateTime mLastGamepadUpdate;

	bool mAddKeys;
	lcTool mTool;
	lcTransformType mTransformType;
	bool mMoveSnapEnabled;
	bool mAngleSnapEnabled;
	int mMoveXYSnapIndex;
	int mMoveZSnapIndex;
	int mAngleSnapIndex;
	bool mRelativeTransform;
	PieceInfo* mCurrentPieceInfo;
	lcSelectionMode mSelectionMode;
	int mModelTabWidgetContextMenuIndex;

	QAction* mActionFileRecentSeparator;

	lcTabWidget* mModelTabWidget;
	QToolBar* mStandardToolBar;
/*** QToolBar* mToolsToolBar;     LPub3D Mod -  move to public ***/
/*** LPub3D Mod - coord format ***/
    lcRotateStepCoordType mRotateStepCoordType;
    lcVector3 mExistingRotStep;
    QString mRotStepTransform;
/*** LPub3D Mod end ***/

	QToolBar* mTimeToolBar;
	QDockWidget* mPartsToolBar;
	QDockWidget* mColorsToolBar;
	QDockWidget* mPropertiesToolBar;
	QDockWidget* mTimelineToolBar;

	lcPartSelectionWidget* mPartSelectionWidget;
	lcQColorList* mColorList;
	lcQPropertiesTree* mPropertiesWidget;
	lcTimelineWidget* mTimelineWidget;
	QLineEdit* mTransformXEdit;
	QLineEdit* mTransformYEdit;
	QLineEdit* mTransformZEdit;

	QLabel* mStatusBarLabel;
	QLabel* mStatusSnapLabel;
/*** LPub3D Mod - disable position/time status ***/
	//QLabel* mStatusPositionLabel;
	//QLabel* mStatusTimeLabel;
/*** LPub3D Mod end ***/

	QMenu* mToolsMenu;
	QMenu* mViewpointMenu;
	QMenu* mCameraMenu;
	QMenu* mProjectionMenu;
	QMenu* mShadingMenu;
	QMenu* mSelectionModeMenu;

/*** LPub3D Mod - designate friend class ***/
	friend class Gui;
/**** LPub3D Mod end ***/
};

extern class lcMainWindow* gMainWindow;
