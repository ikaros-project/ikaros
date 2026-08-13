// Ikaros 3.0

#include "ikaros.h"

namespace ikaros
{
    connection_map
    Kernel::BuildIncomingConnections()
    {
        connection_map incoming_connections;
        for(auto & connection : connections)
            incoming_connections[connection.target].push_back(&connection);
        return incoming_connections;
    }


    std::vector<std::string>
    Kernel::PendingBufferSizes(input_map incoming_connections)
    {
        std::vector<std::string> pending;
        for(auto & [name, component] : components)
        {
            for(dictionary input : component->info_["inputs"])
            {
                std::string full_name = name + "." + input["name"].as_string();
                if(!input.is_set("optional") && incoming_connections.count(full_name) &&
                   buffers.at(full_name).is_uninitialized())
                    pending.push_back(full_name);
            }

            bool is_module = dynamic_cast<Module *>(component.get()) != nullptr;
            for(dictionary output : component->info_["outputs"])
            {
                std::string full_name = name + "." + output["name"].as_string();
                if((is_module || incoming_connections.count(full_name)) &&
                   buffers.at(full_name).is_uninitialized())
                    pending.push_back(full_name);
            }

            for(dictionary state : component->info_["states"])
            {
                std::string full_name = name + "." + state["name"].as_string();
                if(state_buffers.count(full_name) && buffers.at(full_name).is_uninitialized())
                    pending.push_back(full_name);
            }
        }
        return pending;
    }


    std::size_t
    Kernel::BufferSizeSignature() const
    {
        std::size_t signature = 0;
        for(const auto & [name, buffer] : buffers)
        {
            std::size_t local = std::hash<std::string>{}(name);
            local ^= std::hash<int>{}(buffer.rank()) + 0x9e3779b9 +
                     (local << 6) + (local >> 2);
            for(int dimension : buffer.shape())
                local ^= std::hash<int>{}(dimension) + 0x9e3779b9 +
                         (local << 6) + (local >> 2);
            signature ^= local + 0x9e3779b9 + (signature << 6) + (signature >> 2);
        }
        return signature;
    }


    void
    Kernel::PropagateBufferSizes(input_map incoming_connections)
    {
        std::size_t previous_pending = PendingBufferSizes(incoming_connections).size();
        std::size_t previous_signature = BufferSizeSignature();
        for(std::size_t iteration = 0; iteration < components.size(); ++iteration)
        {
            for(auto & [name, component] : components)
            {
                (void)name;
                component->SetSizes(incoming_connections);
            }

            std::size_t pending = PendingBufferSizes(incoming_connections).size();
            std::size_t signature = BufferSizeSignature();
            if(signature == previous_signature && pending == previous_pending)
                break;
            previous_pending = pending;
            previous_signature = signature;
        }
    }


    void
    Kernel::CalculateSizes()
    {
        try
        {
            connection_map incoming_connections = BuildIncomingConnections();
            PropagateBufferSizes(incoming_connections);

            for(auto & [name, component] : components)
            {
                (void)name;
                component->CheckRequiredInputs();
            }

            std::vector<std::string> pending = PendingBufferSizes(incoming_connections);
            if(!pending.empty())
                throw setup_failed("Could not resolve all input and output sizes. " +
                                   std::to_string(pending.size()) +
                                   " buffers remain unresolved: " + join(", ", pending) + ".");
        }
        catch(fatal_error & e)
        {
            throw setup_failed("Could not calculate input and output sizes. " + e.message(), e.path());
        }
        catch(setup_failed & e)
        {
            throw setup_failed("Could not calculate input and output sizes. " + e.message(), e.path());
        }
        catch(const std::exception & e)
        {
            throw setup_failed("Could not calculate input and output sizes. " + std::string(e.what()));
        }
        catch(...)
        {
            throw setup_failed("Could not calculate input and output sizes.");
        }
    }
}
