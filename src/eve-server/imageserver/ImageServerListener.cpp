/*
    ------------------------------------------------------------------------------------
    LICENSE:
    ------------------------------------------------------------------------------------
    This file is part of EVEmu: EVE Online Server Emulator
    Copyright 2006 - 2021 The EVEmu Team
    For the latest information visit https://evemu.dev
    ------------------------------------------------------------------------------------
    This program is free software; you can redistribute it and/or modify it under
    the terms of the GNU Lesser General Public License as published by the Free Software
    Foundation; either version 2 of the License, or (at your option) any later
    version.

    This program is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License along with
    this program; if not, write to the Free Software Foundation, Inc., 59 Temple
    Place - Suite 330, Boston, MA 02111-1307, USA, or go to
    http://www.gnu.org/copyleft/lesser.txt.
    ------------------------------------------------------------------------------------
    Author:        caytchen
*/

#include "eve-server.h"

#include "EVEServerConfig.h"
#include "imageserver/ImageServerListener.h"

namespace {
    boost::asio::ip::tcp::endpoint make_image_listen_endpoint()
    {
        namespace ip = boost::asio::ip;
        const uint16 port = sConfig.net.imageServerPort;
        if (sConfig.net.listenAddress.empty())
            return ip::tcp::endpoint(ip::tcp::v4(), port);

        boost::system::error_code ec;
        const ip::address_v4 addr = ip::address_v4::from_string(sConfig.net.listenAddress, ec);
        if (ec) {
            sLog.Error("ImageServerListener", "Invalid net.listenAddress '%s': %s — binding 0.0.0.0:%u",
                sConfig.net.listenAddress.c_str(), ec.message().c_str(), (unsigned)port);
            return ip::tcp::endpoint(ip::tcp::v4(), port);
        }
        sLog.Blue("ImageServerListener", "Image TCP listening on %s:%u", sConfig.net.listenAddress.c_str(), (unsigned)port);
        return ip::tcp::endpoint(addr, port);
    }
}

ImageServerListener::ImageServerListener(boost::asio::io_context& io)
{
    const proto::endpoint ep = make_image_listen_endpoint();
#if BOOST_VERSION >= 107400
    _acceptor = new boost::asio::basic_socket_acceptor<proto>(io, ep);
#else
    _acceptor = new proto::acceptor(io, ep);
#endif
    StartAccept();
}

ImageServerListener::~ImageServerListener()
{
    delete _acceptor;
}

void ImageServerListener::StartAccept()
{
#if BOOST_VERSION >= 107400
    boost::asio::any_io_executor e = _acceptor->get_executor();
    boost::asio::execution_context &e_context = e.context();
    auto &context_instance = reinterpret_cast<boost::asio::io_context&>(e_context);
#else
    boost::asio::executor e = _acceptor->get_executor();
    boost::asio::execution_context &e_context = e.context();
    boost::asio::io_context &context_instance = static_cast<boost::asio::io_context&>(e_context);
#endif

    std::shared_ptr<ImageServerConnection> connection = ImageServerConnection::create(context_instance);
    _acceptor->async_accept(connection->socket(), std::bind(&ImageServerListener::HandleAccept, this, connection));
}

void ImageServerListener::HandleAccept(std::shared_ptr<ImageServerConnection> connection)
{
    connection->Process();
    StartAccept();
}
