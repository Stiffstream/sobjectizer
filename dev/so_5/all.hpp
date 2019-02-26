/*
 * SObjectizer-5
 */

/*!
 * \since
 * v.5.4.0
 *
 * \file
 * \brief A helper header file for including all public SObjectizer stuff.
 */

#pragma once

#include <so_5/rt.hpp>
#include <so_5/api.hpp>
#include <so_5/wrapped_env.hpp>
#include <so_5/env_infrastructures.hpp>

#include <so_5/enveloped_msg.hpp>

#include <so_5/mchain_helper_functions.hpp>
#include <so_5/thread_helper_functions.hpp>

#include <so_5/disp/one_thread/pub.hpp>
#include <so_5/disp/active_obj/pub.hpp>
//FIXME: uncomment this after implementation of the corresponding dispatchers.
#if 0
#include <so_5/disp/active_group/pub.hpp>
#include <so_5/disp/thread_pool/pub.hpp>
#include <so_5/disp/adv_thread_pool/pub.hpp>
#include <so_5/disp/prio_one_thread/strictly_ordered/pub.hpp>
#include <so_5/disp/prio_one_thread/quoted_round_robin/pub.hpp>
#include <so_5/disp/prio_dedicated_threads/one_per_prio/pub.hpp>
#endif

#include <so_5/version.hpp>

