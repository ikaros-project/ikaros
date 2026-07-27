// Ikaros 3.0

#include "ikaros.h"

namespace ikaros
{
/*************************
 * 
 *  Task sorting
 * 
 *************************/

    bool
    Kernel::HasCycle(const std::vector<std::string> & nodes, const std::vector<std::pair<std::string, std::string>> & edges)
    {
        std::unordered_map<std::string, std::vector<std::string>> graph;
        for(const auto & edge : edges)
            graph[edge.first].push_back(edge.second);

        enum class VisitState
        {
            unvisited,
            visiting,
            visited,
        };

        struct TraversalFrame
        {
            std::string node;
            size_t next_neighbor = 0;
        };

        std::unordered_map<std::string, VisitState> states;
        std::vector<TraversalFrame> traversal;

        for(const std::string & node : nodes)
        {
            if(states[node] != VisitState::unvisited)
                continue;

            states[node] = VisitState::visiting;
            traversal.push_back({node, 0});
            while(!traversal.empty())
            {
                TraversalFrame & frame = traversal.back();
                auto neighbors = graph.find(frame.node);
                if(neighbors == graph.end() || frame.next_neighbor >= neighbors->second.size())
                {
                    states[frame.node] = VisitState::visited;
                    traversal.pop_back();
                    continue;
                }

                const std::string & neighbor = neighbors->second[frame.next_neighbor++];
                if(states[neighbor] == VisitState::visiting)
                    return true;
                if(states[neighbor] == VisitState::unvisited)
                {
                    states[neighbor] = VisitState::visiting;
                    traversal.push_back({neighbor, 0});
                }
            }
        }

        return false;
    }


    std::vector<std::vector<std::string>>
    Kernel::FindSubgraphs(const std::vector<std::string> & nodes, const std::vector<std::pair<std::string, std::string>> & edges)
    {
        std::unordered_map<std::string, std::vector<std::string>> graph;
        for(const auto & edge : edges)
        {
            graph[edge.first].push_back(edge.second);
            graph[edge.second].push_back(edge.first);
        }

        struct TraversalFrame
        {
            std::string node;
            size_t next_neighbor = 0;
        };

        std::unordered_set<std::string> visited;
        std::vector<std::vector<std::string>> components;

        for(const std::string & node : nodes)
        {
            if(!visited.insert(node).second)
                continue;

            std::vector<std::string> component{node};
            std::vector<TraversalFrame> traversal{{node, 0}};
            while(!traversal.empty())
            {
                TraversalFrame & frame = traversal.back();
                auto neighbors = graph.find(frame.node);
                if(neighbors == graph.end() || frame.next_neighbor >= neighbors->second.size())
                {
                    traversal.pop_back();
                    continue;
                }

                const std::string & neighbor = neighbors->second[frame.next_neighbor++];
                if(visited.insert(neighbor).second)
                {
                    component.push_back(neighbor);
                    traversal.push_back({neighbor, 0});
                }
            }
            components.push_back(std::move(component));
        }

        return components;
    }


    std::vector<std::string>
    Kernel::TopologicalSort(const std::vector<std::string> & component, const std::unordered_map<std::string, std::vector<std::string>> & graph)
    {
        struct TraversalFrame
        {
            std::string node;
            size_t next_neighbor = 0;
        };

        std::unordered_set<std::string> visited;
        std::vector<std::string> finished;

        for(const std::string & node : component)
        {
            if(!visited.insert(node).second)
                continue;

            std::vector<TraversalFrame> traversal{{node, 0}};
            while(!traversal.empty())
            {
                TraversalFrame & frame = traversal.back();
                auto neighbors = graph.find(frame.node);
                if(neighbors == graph.end() || frame.next_neighbor >= neighbors->second.size())
                {
                    finished.push_back(frame.node);
                    traversal.pop_back();
                    continue;
                }

                const std::string & neighbor = neighbors->second[frame.next_neighbor++];
                if(visited.insert(neighbor).second)
                    traversal.push_back({neighbor, 0});
            }
        }

        std::vector<std::string> sorted_subgraph;
        sorted_subgraph.reserve(finished.size());
        for(auto node = finished.rbegin(); node != finished.rend(); ++node)
            sorted_subgraph.push_back(*node);
        return sorted_subgraph;
    }


    std::vector<std::vector<std::string>>
    Kernel::Sort(const std::vector<std::string> & nodes, const std::vector<std::pair<std::string, std::string>> & edges)
    {
        if(HasCycle(nodes, edges))
            throw setup_failed("Network has zero-delay loops");

        std::vector<std::vector<std::string>> components = FindSubgraphs(nodes, edges);

        std::unordered_map<std::string, std::vector<std::string>> graph;
        for(const auto & edge : edges)
            graph[edge.first].push_back(edge.second);

        std::vector<std::vector<std::string>> result;
        result.reserve(components.size());
        for(const auto & component : components)
            result.push_back(TopologicalSort(component, graph));
        return result;
    }



    void
    Kernel::SortTasks()
    {
        std::vector<std::string> nodes;
        std::vector<std::pair<std::string, std::string>> arcs;
        std::map<std::string, Task *> task_map;

        for(auto & [s,c] : components)
        {
            nodes.push_back(s);
            task_map[s] = c.get(); // Save in task map
        }

        for(size_t connection_index = 0; connection_index < connections.size(); ++connection_index)
        {
            auto & c = connections[connection_index];
            if(!c.HasZeroDelay())
                continue;

            std::string s = peek_rhead(c.source,".");
            std::string t = peek_rhead(c.target,".");
            std::string cc = "CON(" + std::to_string(connection_index) + ")";

            nodes.push_back(cc);
            arcs.push_back({s, cc});
            arcs.push_back({cc, t});
            task_map[cc] = &c; // Save in task map
        }

        auto r = Sort(nodes, arcs);

        // Fill task list

        tasks.clear();
        for(auto s : r)
        {
            std::vector<Task *> task_list;
            bool priority_task = false;
            for(auto ss: s)
            {
                if(task_map[ss]->Priority())
                    priority_task = true;
                task_list.push_back(task_map[ss]); // Get task pointer here
            }
            if(priority_task)
                tasks.insert(tasks.begin(), task_list);
            else
                tasks.push_back(task_list);

        }
    }



}
