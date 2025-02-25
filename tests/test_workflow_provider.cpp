/*
 * Copyright (C) 2021-2022 Canonical, Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "common.h"
#include "mock_logger.h"
#include "mock_poco_zip_utils.h"
#include "mock_url_downloader.h"
#include "path.h"
#include "temp_dir.h"

#include <multipass/default_vm_workflow_provider.h>
#include <multipass/exceptions/download_exception.h>
#include <multipass/exceptions/workflow_exceptions.h>
#include <multipass/memory_size.h>
#include <multipass/url_downloader.h>
#include <multipass/utils.h>

#include <Poco/Exception.h>

#include <QFileInfo>

#include <chrono>

namespace mp = multipass;
namespace mpl = multipass::logging;
namespace mpt = multipass::test;

using namespace std::chrono_literals;
using namespace testing;

namespace
{
const QString test_workflows_zip{"/test-workflows.zip"};
const QString multipass_workflows_zip{"/multipass-workflows.zip"};

struct VMWorkflowProvider : public Test
{
    QString workflows_zip_url{QUrl::fromLocalFile(mpt::test_data_path()).toString() + test_workflows_zip};
    mp::URLDownloader url_downloader{std::chrono::seconds(10)};
    mpt::TempDir cache_dir;
    std::chrono::seconds default_ttl{1s};
    mpt::MockLogger::Scope logger_scope = mpt::MockLogger::inject();
};
} // namespace

TEST_F(VMWorkflowProvider, downloadsZipToExpectedLocation)
{
    mp::DefaultVMWorkflowProvider workflow_provider{workflows_zip_url, &url_downloader, cache_dir.path(), default_ttl};

    const QFileInfo original_zip{mpt::test_data_path() + test_workflows_zip};
    const QFileInfo downloaded_zip{cache_dir.path() + multipass_workflows_zip};

    EXPECT_TRUE(downloaded_zip.exists());
    EXPECT_EQ(downloaded_zip.size(), original_zip.size());
}

TEST_F(VMWorkflowProvider, fetchWorkflowForUnknownWorkflowThrows)
{
    mp::DefaultVMWorkflowProvider workflow_provider{workflows_zip_url, &url_downloader, cache_dir.path(), default_ttl};

    mp::VirtualMachineDescription vm_desc{0, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}};

    EXPECT_THROW(workflow_provider.fetch_workflow_for("phony", vm_desc), std::out_of_range);
}

TEST_F(VMWorkflowProvider, infoForUnknownWorkflowThrows)
{
    mp::DefaultVMWorkflowProvider workflow_provider{workflows_zip_url, &url_downloader, cache_dir.path(), default_ttl};

    EXPECT_THROW(workflow_provider.info_for("phony"), std::out_of_range);
}

TEST_F(VMWorkflowProvider, invalidImageSchemeThrows)
{
    mp::DefaultVMWorkflowProvider workflow_provider{workflows_zip_url, &url_downloader, cache_dir.path(), default_ttl};

    mp::VirtualMachineDescription vm_desc{0, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}};

    MP_EXPECT_THROW_THAT(workflow_provider.fetch_workflow_for("invalid-image-workflow", vm_desc),
                         mp::InvalidWorkflowException, mpt::match_what(StrEq("Unsupported image scheme in Workflow")));
}

TEST_F(VMWorkflowProvider, invalidMinCoresThrows)
{
    mp::DefaultVMWorkflowProvider workflow_provider{workflows_zip_url, &url_downloader, cache_dir.path(), default_ttl};

    mp::VirtualMachineDescription vm_desc{0, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}};

    MP_EXPECT_THROW_THAT(workflow_provider.fetch_workflow_for("invalid-cpu-workflow", vm_desc),
                         mp::InvalidWorkflowException,
                         mpt::match_what(StrEq("Minimum CPU value in workflow is invalid")));
}

TEST_F(VMWorkflowProvider, invalidMinMemorySizeThrows)
{
    mp::DefaultVMWorkflowProvider workflow_provider{workflows_zip_url, &url_downloader, cache_dir.path(), default_ttl};

    mp::VirtualMachineDescription vm_desc{0, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}};

    MP_EXPECT_THROW_THAT(workflow_provider.fetch_workflow_for("invalid-memory-size-workflow", vm_desc),
                         mp::InvalidWorkflowException,
                         mpt::match_what(StrEq("Minimum memory size value in workflow is invalid")));
}

TEST_F(VMWorkflowProvider, invalidMinDiskSpaceThrows)
{
    mp::DefaultVMWorkflowProvider workflow_provider{workflows_zip_url, &url_downloader, cache_dir.path(), default_ttl};

    mp::VirtualMachineDescription vm_desc{0, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}};

    MP_EXPECT_THROW_THAT(workflow_provider.fetch_workflow_for("invalid-disk-space-workflow", vm_desc),
                         mp::InvalidWorkflowException,
                         mpt::match_what(StrEq("Minimum disk space value in workflow is invalid")));
}

TEST_F(VMWorkflowProvider, fetchTestWorkflow1ReturnsExpectedInfo)
{
    mp::DefaultVMWorkflowProvider workflow_provider{workflows_zip_url, &url_downloader, cache_dir.path(), default_ttl};

    mp::VirtualMachineDescription vm_desc{0, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}};

    auto query = workflow_provider.fetch_workflow_for("test-workflow1", vm_desc);

    auto yaml_as_str = mp::utils::emit_yaml(vm_desc.vendor_data_config);

    EXPECT_EQ(query.release, "default");
    EXPECT_EQ(vm_desc.num_cores, 2);
    EXPECT_EQ(vm_desc.mem_size, mp::MemorySize("2G"));
    EXPECT_EQ(vm_desc.disk_space, mp::MemorySize("25G"));
    EXPECT_THAT(yaml_as_str, AllOf(HasSubstr("runcmd"), HasSubstr("echo \"Have fun!\"")));
}

TEST_F(VMWorkflowProvider, fetchTestWorkflow2ReturnsExpectedInfo)
{
    mp::DefaultVMWorkflowProvider workflow_provider{workflows_zip_url, &url_downloader, cache_dir.path(), default_ttl};

    mp::VirtualMachineDescription vm_desc{0, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}};

    auto query = workflow_provider.fetch_workflow_for("test-workflow2", vm_desc);

    EXPECT_EQ(query.release, "bionic");
    EXPECT_EQ(query.remote_name, "daily");
    EXPECT_EQ(vm_desc.num_cores, 4);
    EXPECT_EQ(vm_desc.mem_size, mp::MemorySize("4G"));
    EXPECT_EQ(vm_desc.disk_space, mp::MemorySize("50G"));
    EXPECT_TRUE(vm_desc.vendor_data_config.IsNull());
}

TEST_F(VMWorkflowProvider, missingDescriptionThrows)
{
    mp::DefaultVMWorkflowProvider workflow_provider{workflows_zip_url, &url_downloader, cache_dir.path(), default_ttl};

    const std::string workflow{"missing-description-workflow"};
    MP_EXPECT_THROW_THAT(
        workflow_provider.info_for(workflow), mp::InvalidWorkflowException,
        mpt::match_what(StrEq(fmt::format("The \'description\' key is required for the {} workflow", workflow))));
}

TEST_F(VMWorkflowProvider, missingVersionThrows)
{
    mp::DefaultVMWorkflowProvider workflow_provider{workflows_zip_url, &url_downloader, cache_dir.path(), default_ttl};

    const std::string workflow{"missing-version-workflow"};
    MP_EXPECT_THROW_THAT(
        workflow_provider.info_for(workflow), mp::InvalidWorkflowException,
        mpt::match_what(StrEq(fmt::format("The \'version\' key is required for the {} workflow", workflow))));
}

TEST_F(VMWorkflowProvider, invalidDescriptionThrows)
{
    mp::DefaultVMWorkflowProvider workflow_provider{workflows_zip_url, &url_downloader, cache_dir.path(), default_ttl};

    const std::string workflow{"invalid-description-workflow"};
    MP_EXPECT_THROW_THAT(
        workflow_provider.info_for(workflow), mp::InvalidWorkflowException,
        mpt::match_what(StrEq(fmt::format("Cannot convert \'description\' key for the {} workflow", workflow))));
}

TEST_F(VMWorkflowProvider, invalidVersionThrows)
{
    mp::DefaultVMWorkflowProvider workflow_provider{workflows_zip_url, &url_downloader, cache_dir.path(), default_ttl};

    const std::string workflow{"invalid-version-workflow"};
    MP_EXPECT_THROW_THAT(
        workflow_provider.info_for(workflow), mp::InvalidWorkflowException,
        mpt::match_what(StrEq(fmt::format("Cannot convert \'version\' key for the {} workflow", workflow))));
}

TEST_F(VMWorkflowProvider, invalidCloudInitThrows)
{
    mp::DefaultVMWorkflowProvider workflow_provider{workflows_zip_url, &url_downloader, cache_dir.path(), default_ttl};

    mp::VirtualMachineDescription vm_desc{0, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}};

    const std::string workflow{"invalid-cloud-init-workflow"};

    MP_EXPECT_THROW_THAT(
        workflow_provider.fetch_workflow_for(workflow, vm_desc), mp::InvalidWorkflowException,
        mpt::match_what(StrEq(fmt::format("Cannot convert cloud-init data for the {} workflow", workflow))));
}

TEST_F(VMWorkflowProvider, givenCoresLessThanMinimumThrows)
{
    mp::DefaultVMWorkflowProvider workflow_provider{workflows_zip_url, &url_downloader, cache_dir.path(), default_ttl};

    mp::VirtualMachineDescription vm_desc{1, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}};

    MP_EXPECT_THROW_THAT(workflow_provider.fetch_workflow_for("test-workflow1", vm_desc), mp::WorkflowMinimumException,
                         mpt::match_what(AllOf(HasSubstr("Number of CPUs"), HasSubstr("2"))));
}

TEST_F(VMWorkflowProvider, givenMemLessThanMinimumThrows)
{
    mp::DefaultVMWorkflowProvider workflow_provider{workflows_zip_url, &url_downloader, cache_dir.path(), default_ttl};

    mp::VirtualMachineDescription vm_desc{0, mp::MemorySize{"1G"}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}};

    MP_EXPECT_THROW_THAT(workflow_provider.fetch_workflow_for("test-workflow1", vm_desc), mp::WorkflowMinimumException,
                         mpt::match_what(AllOf(HasSubstr("Memory size"), HasSubstr("2G"))));
}

TEST_F(VMWorkflowProvider, givenDiskSpaceLessThanMinimumThrows)
{
    mp::DefaultVMWorkflowProvider workflow_provider{workflows_zip_url, &url_downloader, cache_dir.path(), default_ttl};

    mp::VirtualMachineDescription vm_desc{0, {}, mp::MemorySize{"20G"}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}};

    MP_EXPECT_THROW_THAT(workflow_provider.fetch_workflow_for("test-workflow1", vm_desc), mp::WorkflowMinimumException,
                         mpt::match_what(AllOf(HasSubstr("Disk space"), HasSubstr("25G"))));
}

TEST_F(VMWorkflowProvider, higherOptionsIsNotOverriden)
{
    mp::DefaultVMWorkflowProvider workflow_provider{workflows_zip_url, &url_downloader, cache_dir.path(), default_ttl};

    mp::VirtualMachineDescription vm_desc{
        4, mp::MemorySize{"4G"}, mp::MemorySize{"50G"}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}};

    workflow_provider.fetch_workflow_for("test-workflow1", vm_desc);

    EXPECT_EQ(vm_desc.num_cores, 4);
    EXPECT_EQ(vm_desc.mem_size, mp::MemorySize("4G"));
    EXPECT_EQ(vm_desc.disk_space, mp::MemorySize("50G"));
}

TEST_F(VMWorkflowProvider, infoForReturnsExpectedInfo)
{
    mp::DefaultVMWorkflowProvider workflow_provider{workflows_zip_url, &url_downloader, cache_dir.path(), default_ttl};

    auto workflow = workflow_provider.info_for("test-workflow2");

    ASSERT_EQ(workflow.aliases.size(), 1);
    EXPECT_EQ(workflow.aliases[0], "test-workflow2");
    EXPECT_EQ(workflow.release_title, "Another test workflow");
    EXPECT_EQ(workflow.version, "0.1");
}

TEST_F(VMWorkflowProvider, allWorkflowsReturnsExpectedInfo)
{
    logger_scope.mock_logger->screen_logs(mpl::Level::error);
    logger_scope.mock_logger->expect_log(
        mpl::Level::error,
        "Invalid workflow: Cannot convert 'description' key for the invalid-description-workflow workflow");
    logger_scope.mock_logger->expect_log(
        mpl::Level::error, "Invalid workflow: Cannot convert 'version' key for the invalid-version-workflow workflow");
    logger_scope.mock_logger->expect_log(
        mpl::Level::error,
        "Invalid workflow: The 'description' key is required for the missing-description-workflow workflow");
    logger_scope.mock_logger->expect_log(
        mpl::Level::error, "Invalid workflow: The 'version' key is required for the missing-version-workflow workflow");
    logger_scope.mock_logger->expect_log(
        mpl::Level::error, "Invalid workflow name \'42-invalid-hostname-workflow\': must be a valid host name");
    logger_scope.mock_logger->expect_log(
        mpl::Level::error, "Invalid workflow: Cannot convert 'runs-on' key for the invalid-arch workflow");

    mp::DefaultVMWorkflowProvider workflow_provider{workflows_zip_url, &url_downloader, cache_dir.path(), default_ttl};

    auto workflows = workflow_provider.all_workflows();

    EXPECT_EQ(workflows.size(), 10ul);

    EXPECT_TRUE(std::find_if(workflows.cbegin(), workflows.cend(), [](const mp::VMImageInfo& workflow_info) {
                    return ((workflow_info.aliases.size() == 1) && (workflow_info.aliases[0] == "test-workflow1") &&
                            (workflow_info.release_title == "The first test workflow"));
                }) != workflows.cend());

    EXPECT_TRUE(std::find_if(workflows.cbegin(), workflows.cend(), [](const mp::VMImageInfo& workflow_info) {
                    return ((workflow_info.aliases.size() == 1) && (workflow_info.aliases[0] == "test-workflow2") &&
                            (workflow_info.release_title == "Another test workflow"));
                }) != workflows.cend());
}

TEST_F(VMWorkflowProvider, doesNotUpdateWorkflowsWhenNotNeeded)
{
    mpt::MockURLDownloader mock_url_downloader;

    EXPECT_CALL(mock_url_downloader, download_to(_, _, _, _, _))
        .Times(1)
        .WillRepeatedly([](auto, const QString& file_name, auto...) {
            QFile file(file_name);
            file.open(QFile::WriteOnly);
        });

    mp::DefaultVMWorkflowProvider workflow_provider{workflows_zip_url, &mock_url_downloader, cache_dir.path(),
                                                    default_ttl};

    workflow_provider.all_workflows();
}

TEST_F(VMWorkflowProvider, updatesWorkflowsWhenNeeded)
{
    mpt::MockURLDownloader mock_url_downloader;
    EXPECT_CALL(mock_url_downloader, download_to(_, _, _, _, _))
        .Times(2)
        .WillRepeatedly([](auto, const QString& file_name, auto...) {
            QFile file(file_name);

            if (!file.exists())
                file.open(QFile::WriteOnly);
        });

    mp::DefaultVMWorkflowProvider workflow_provider{workflows_zip_url, &mock_url_downloader, cache_dir.path(),
                                                    std::chrono::milliseconds(0)};

    workflow_provider.all_workflows();
}

TEST_F(VMWorkflowProvider, downloadFailureOnStartupLogsErrorAndDoesNotThrow)
{
    const std::string error_msg{"There is a problem, Houston."};
    const std::string url{"https://fake.url"};
    mpt::MockURLDownloader mock_url_downloader;
    EXPECT_CALL(mock_url_downloader, download_to(_, _, _, _, _)).WillOnce(Throw(mp::DownloadException(url, error_msg)));

    auto logger_scope = mpt::MockLogger::inject();
    logger_scope.mock_logger->screen_logs(mpl::Level::error);
    logger_scope.mock_logger->expect_log(
        mpl::Level::error, fmt::format("Error fetching workflows: failed to download from '{}': {}", url, error_msg));

    EXPECT_NO_THROW(
        mp::DefaultVMWorkflowProvider(workflows_zip_url, &mock_url_downloader, cache_dir.path(), default_ttl));
}

TEST_F(VMWorkflowProvider, downloadFailureDuringUpdateLogsErrorAndDoesNotThrow)
{
    const std::string error_msg{"There is a problem, Houston."};
    const std::string url{"https://fake.url"};
    mpt::MockURLDownloader mock_url_downloader;
    EXPECT_CALL(mock_url_downloader, download_to(_, _, _, _, _))
        .Times(2)
        .WillOnce([](auto, const QString& file_name, auto...) {
            QFile file(file_name);
            file.open(QFile::WriteOnly);
        })
        .WillRepeatedly(Throw(mp::DownloadException(url, error_msg)));

    auto logger_scope = mpt::MockLogger::inject();
    logger_scope.mock_logger->screen_logs(mpl::Level::error);
    logger_scope.mock_logger->expect_log(
        mpl::Level::error, fmt::format("Error fetching workflows: failed to download from '{}': {}", url, error_msg));

    mp::DefaultVMWorkflowProvider workflow_provider{workflows_zip_url, &mock_url_downloader, cache_dir.path(),
                                                    std::chrono::milliseconds(0)};

    EXPECT_NO_THROW(workflow_provider.all_workflows());
}

TEST_F(VMWorkflowProvider, zipArchivePocoExceptionLogsErrorAndDoesNotThrow)
{
    auto [mock_poco_zip_utils, guard] = mpt::MockPocoZipUtils::inject();
    const std::string error_msg{"Rubbish zip file"};

    EXPECT_CALL(*mock_poco_zip_utils, zip_archive_for(_)).WillOnce(Throw(Poco::IllegalStateException(error_msg)));

    auto logger_scope = mpt::MockLogger::inject();
    logger_scope.mock_logger->screen_logs(mpl::Level::error);
    logger_scope.mock_logger->expect_log(
        mpl::Level::error, fmt::format("Error extracting Workflows zip file: Illegal state: {}", error_msg));

    EXPECT_NO_THROW(mp::DefaultVMWorkflowProvider(workflows_zip_url, &url_downloader, cache_dir.path(),
                                                  std::chrono::milliseconds(0)));
}

TEST_F(VMWorkflowProvider, generalExceptionDuringStartupThrows)
{
    const std::string error_msg{"Bad stuff just happened"};
    mpt::MockURLDownloader mock_url_downloader;
    EXPECT_CALL(mock_url_downloader, download_to(_, _, _, _, _)).WillRepeatedly(Throw(std::runtime_error(error_msg)));

    MP_EXPECT_THROW_THAT(mp::DefaultVMWorkflowProvider workflow_provider(
                             workflows_zip_url, &mock_url_downloader, cache_dir.path(), std::chrono::milliseconds(0)),
                         std::runtime_error, mpt::match_what(StrEq(error_msg)));
}

TEST_F(VMWorkflowProvider, generalExceptionDuringCallThrows)
{
    const std::string error_msg{"This can't be possible"};
    mpt::MockURLDownloader mock_url_downloader;
    EXPECT_CALL(mock_url_downloader, download_to(_, _, _, _, _))
        .Times(2)
        .WillOnce([](auto, const QString& file_name, auto...) {
            QFile file(file_name);
            file.open(QFile::WriteOnly);
        })
        .WillRepeatedly(Throw(std::runtime_error(error_msg)));

    mp::DefaultVMWorkflowProvider workflow_provider(workflows_zip_url, &mock_url_downloader, cache_dir.path(),
                                                    std::chrono::milliseconds(0));

    MP_EXPECT_THROW_THAT(workflow_provider.info_for("foo"), std::runtime_error, mpt::match_what(StrEq(error_msg)));
}

TEST_F(VMWorkflowProvider, validWorkflowReturnsExpectedName)
{
    const std::string workflow_name{"test-workflow1"};

    mp::DefaultVMWorkflowProvider workflow_provider{workflows_zip_url, &url_downloader, cache_dir.path(), default_ttl};

    auto name = workflow_provider.name_from_workflow(workflow_name);

    EXPECT_EQ(name, workflow_name);
}

TEST_F(VMWorkflowProvider, nonexistentWorkflowReturnsEmptyName)
{
    const std::string workflow_name{"not-a-workflow"};

    mp::DefaultVMWorkflowProvider workflow_provider{workflows_zip_url, &url_downloader, cache_dir.path(), default_ttl};

    auto name = workflow_provider.name_from_workflow(workflow_name);

    EXPECT_TRUE(name.empty());
}

TEST_F(VMWorkflowProvider, returnsExpectedTimeout)
{
    mp::DefaultVMWorkflowProvider workflow_provider{workflows_zip_url, &url_downloader, cache_dir.path(), default_ttl};

    EXPECT_EQ(workflow_provider.workflow_timeout("test-workflow1"), 600);
}

TEST_F(VMWorkflowProvider, noTimeoutReturnsZero)
{
    mp::DefaultVMWorkflowProvider workflow_provider{workflows_zip_url, &url_downloader, cache_dir.path(), default_ttl};

    EXPECT_EQ(workflow_provider.workflow_timeout("test-workflow2"), 0);
}

TEST_F(VMWorkflowProvider, nonexistentWorkflowTimeoutReturnsZero)
{
    mp::DefaultVMWorkflowProvider workflow_provider{workflows_zip_url, &url_downloader, cache_dir.path(), default_ttl};

    EXPECT_EQ(workflow_provider.workflow_timeout("not-a-workflow"), 0);
}

TEST_F(VMWorkflowProvider, invalidTimeoutThrows)
{
    mp::DefaultVMWorkflowProvider workflow_provider{workflows_zip_url, &url_downloader, cache_dir.path(), default_ttl};

    MP_EXPECT_THROW_THAT(workflow_provider.workflow_timeout("invalid-timeout-workflow"), mp::InvalidWorkflowException,
                         mpt::match_what(StrEq(fmt::format("Invalid timeout given in workflow"))));
}

TEST_F(VMWorkflowProvider, noImageDefinedReturnsDefault)
{
    mp::DefaultVMWorkflowProvider workflow_provider{workflows_zip_url, &url_downloader, cache_dir.path(), default_ttl};

    mp::VirtualMachineDescription vm_desc{0, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}};

    auto query = workflow_provider.fetch_workflow_for("no-image-workflow", vm_desc);

    EXPECT_EQ(query.release, "default");
}

TEST_F(VMWorkflowProvider, invalidRunsOnThrows)
{
    mp::DefaultVMWorkflowProvider workflow_provider{workflows_zip_url, &url_downloader, cache_dir.path(), default_ttl};

    const std::string workflow{"invalid-description-workflow"};
    MP_EXPECT_THROW_THAT(
        workflow_provider.info_for(workflow), mp::InvalidWorkflowException,
        mpt::match_what(StrEq(fmt::format("Cannot convert \'description\' key for the {} workflow", workflow))));
}

TEST_F(VMWorkflowProvider, fetchInvalidRunsOnThrows)
{
    mp::DefaultVMWorkflowProvider workflow_provider{workflows_zip_url, &url_downloader, cache_dir.path(), default_ttl};

    const std::string workflow{"invalid-arch"};
    MP_EXPECT_THROW_THAT(
        workflow_provider.info_for(workflow), mp::InvalidWorkflowException,
        mpt::match_what(StrEq(fmt::format("Cannot convert \'runs-on\' key for the {} workflow", workflow))));
}

TEST_F(VMWorkflowProvider, infoForIncompatibleThrows)
{
    mp::DefaultVMWorkflowProvider workflow_provider{workflows_zip_url, &url_downloader, cache_dir.path(), default_ttl};

    const std::string workflow{"arch-only"};
    MP_EXPECT_THROW_THAT(workflow_provider.info_for(workflow), mp::IncompatibleWorkflowException,
                         mpt::match_what(StrEq(workflow)));
}

TEST_F(VMWorkflowProvider, infoForCompatibleReturnsExpectedInfo)
{
    mp::DefaultVMWorkflowProvider workflow_provider{workflows_zip_url, &url_downloader, cache_dir.path(), default_ttl,
                                                    "arch"};

    auto workflow = workflow_provider.info_for("arch-only");

    ASSERT_EQ(workflow.aliases.size(), 1);
    EXPECT_EQ(workflow.aliases[0], "arch-only");
    EXPECT_EQ(workflow.release_title, "An arch-only workflow");
}

TEST_F(VMWorkflowProvider, allWorkflowsReturnsExpectedInfoForArch)
{
    mp::DefaultVMWorkflowProvider workflow_provider{workflows_zip_url, &url_downloader, cache_dir.path(), default_ttl,
                                                    "arch"};

    auto workflows = workflow_provider.all_workflows();

    EXPECT_EQ(workflows.size(), 11ul);
    EXPECT_TRUE(std::find_if(workflows.cbegin(), workflows.cend(), [](const mp::VMImageInfo& workflow_info) {
                    return ((workflow_info.aliases.size() == 1) && (workflow_info.aliases[0] == "arch-only") &&
                            (workflow_info.release_title == "An arch-only workflow"));
                }) != workflows.cend());
    ASSERT_EQ(workflows[0].aliases.size(), 1);
}
