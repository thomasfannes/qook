#ifndef HEADER_cook_project_RunConfigurationWidget_hpp_ALREADY_INCLUDED
#define HEADER_cook_project_RunConfigurationWidget_hpp_ALREADY_INCLUDED

#include <projectexplorer/runconfiguration.h>

namespace cook { namespace project {

class RunConfiguration;

class RunSettingsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RunSettingsWidget(RunConfiguration * run_configuration, QWidget * parent = nullptr);

private:
    RunConfiguration * run_config_;
};

} }

#endif
