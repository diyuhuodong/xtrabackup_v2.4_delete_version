/*
  Copyright (c) 2015, 2023, Oracle and/or its affiliates.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License, version 2.0,
  as published by the Free Software Foundation.

  This program is also distributed with certain software (including
  but not limited to OpenSSL) that is licensed under separate terms,
  as designated in a particular file or component or in included license
  documentation.  The authors of MySQL hereby grant you an additional
  permission to link the program and your derivative works with the
  separately licensed software that they have included with MySQL.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License, version 2.0, for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA
*/

#ifndef COMPOSITE_MESSAGE_HANDLER_INCLUDED
#define COMPOSITE_MESSAGE_HANDLER_INCLUDED

#include "i_callable.h"
#include "instance_callback.h"
#include "base/message_data.h"
#include <vector>

namespace Mysql{
namespace Tools{
namespace Dump{

class Composite_message_handler
{
public:
  static Mysql::I_callable<bool, const Mysql::Tools::Base::Message_data&>*
    create_composite_handler
    (const std::vector<Mysql::I_callable<bool,
        const Mysql::Tools::Base::Message_data&>*>&
      message_handlers);

private:
  Composite_message_handler(
    const std::vector<Mysql::I_callable<
      bool, const Mysql::Tools::Base::Message_data&>*>&
    message_handlers);
  /**
    Passes message to message callbacks in reverse order, stopping on first
    handler to declare message consumed.
   */
  bool pass_message(const Mysql::Tools::Base::Message_data& message_data);

  std::vector<Mysql::I_callable<
      bool, const Mysql::Tools::Base::Message_data&>*>
    m_message_handlers;
};

}
}
}

#endif
