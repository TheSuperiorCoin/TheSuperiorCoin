// Copyright (c) 2014-2017, The Superior Project
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#include "wallet/wallet_args.h"

#include <boost/filesystem/path.hpp>
#include <boost/format.hpp>
#include "common/i18n.h"
#include "common/scoped_message_writer.h"
#include "common/util.h"
#include "misc_log_ex.h"
#include "string_tools.h"
#include "version.h"

#if defined(WIN32)
#include <crtdbg.h>
#endif

#undef Superior_DEFAULT_LOG_CATEGORY
#define Superior_DEFAULT_LOG_CATEGORY "wallet.wallet2"

// workaround for a suspected bug in pthread/kernel on MacOS X
#ifdef __APPLE__
#define DEFAULT_MAX_CONCURRENCY 1
#else
#define DEFAULT_MAX_CONCURRENCY 0
#endif


namespace wallet_args
{
  // Create on-demand to prevent static initialization order fiasco issues.
  command_line::arg_descriptor<std::string> arg_generate_from_json()
  {
    return {"generate-from-json", wallet_args::tr("Generate wallet from JSON format file"), ""};
  }
  command_line::arg_descriptor<std::string> arg_wallet_file()
  {
    return {"wallet-file", wallet_args::tr("Use wallet <arg>"), ""};
  }

  const char* tr(const char* str)
  {
    return i18n_translate(str, "wallet_args");
  }

  boost::optional<boost::program_options::variables_map> main(
    int argc, char** argv,
    const char* const usage,
    boost::program_options::options_description desc_params,
    const boost::program_options::positional_options_description& positional_options,
    const char *default_log_name,
    bool log_to_console)
  
  {
    namespace bf = boost::filesystem;
    namespace po = boost::program_options;
#ifdef WIN32
    _CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

    const command_line::arg_descriptor<std::string> arg_log_level = {"log-level", "0-4 or categories", ""};
    const command_line::arg_descriptor<uint32_t> arg_max_concurrency = {"max-concurrency", wallet_args::tr("Max number of threads to use for a parallel job"), DEFAULT_MAX_CONCURRENCY};
    const command_line::arg_descriptor<std::string> arg_log_file = {"log-file", wallet_args::tr("Specify log file"), ""};
    const command_line::arg_descriptor<std::string> arg_config_file = {"config-file", wallet_args::tr("Config file"), "", true};


    std::string lang = i18n_get_language();
    tools::sanitize_locale();
    tools::set_strict_default_file_permissions(true);

    epee::string_tools::set_module_name_and_folder(argv[0]);

    po::options_description desc_general(wallet_args::tr("General options"));
    command_line::add_arg(desc_general, command_line::arg_help);
    command_line::add_arg(desc_general, command_line::arg_version);

    command_line::add_arg(desc_params, arg_log_file, "");
    command_line::add_arg(desc_params, arg_log_level);
    command_line::add_arg(desc_params, arg_max_concurrency);
    command_line::add_arg(desc_params, arg_config_file);

    i18n_set_language("translations", "Superior", lang);

    po::options_description desc_all;
    desc_all.add(desc_general).add(desc_params);
    po::variables_map vm;
    bool r = command_line::handle_error_helper(desc_all, [&]()
    {
      auto parser = po::command_line_parser(argc, argv).options(desc_all).positional(positional_options);
      po::store(parser.run(), vm);

      if(command_line::has_arg(vm, arg_config_file))
      {
        std::string config = command_line::get_arg(vm, arg_config_file);
        bf::path config_path(config);
        boost::system::error_code ec;
        if (bf::exists(config_path, ec))
        {
          po::store(po::parse_config_file<char>(config_path.string<std::string>().c_str(), desc_params), vm);
        }
        else
        {
          tools::fail_msg_writer() << wallet_args::tr("Can't find config file ") << config;
          return false;
        }
      }

      po::notify(vm);
      return true;
    });
    if (!r)
      return boost::none;

    std::string log_path;
    if (!vm["log-file"].defaulted())
      log_path = command_line::get_arg(vm, arg_log_file);
    else
      log_path = mlog_get_default_log_path(default_log_name);
    mlog_configure(log_path, log_to_console);
    if (!vm["log-level"].defaulted())
    {
      mlog_set_log(command_line::get_arg(vm, arg_log_level).c_str());
    }

    if (command_line::get_arg(vm, command_line::arg_help))
    {
      tools::msg_writer() << "Superior '" << Superior_RELEASE_NAME << "' (v" << Superior_VERSION_FULL << ")" << ENDL;
      tools::msg_writer() << wallet_args::tr("This is the command line Superior wallet. It needs to connect to a Superior\n"
												"daemon to work correctly.") << ENDL;
      tools::msg_writer() << wallet_args::tr("Usage:") << ENDL << "  " << usage;
      tools::msg_writer() << desc_all;
      return boost::none;
    }
    else if (command_line::get_arg(vm, command_line::arg_version))
    {
      tools::msg_writer() << "Superior '" << Superior_RELEASE_NAME << "' (v" << Superior_VERSION_FULL << ")";
      return boost::none;
    }

    if(command_line::has_arg(vm, arg_max_concurrency))
      tools::set_max_concurrency(command_line::get_arg(vm, arg_max_concurrency));

    tools::scoped_message_writer(epee::console_color_white, true) << "Superior '" << Superior_RELEASE_NAME << "' (v" << Superior_VERSION_FULL << ")";

    if (!vm["log-level"].defaulted())
      MINFO("Setting log level = " << command_line::get_arg(vm, arg_log_level));
    else
      MINFO("Setting log levels = " << getenv("Superior_LOGS"));
    MINFO(wallet_args::tr("Logging to: ") << log_path);
    tools::scoped_message_writer(epee::console_color_white, true) << boost::format(wallet_args::tr("Logging to %s")) % log_path;

    return {std::move(vm)};
  }
}
