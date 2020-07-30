// Copyright 2018 H-AXE
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <memory>
#include <numeric>
#include <queue>
#include <vector>

#include "glog/logging.h"

#include "common/engine.h"

class Vertex {
    public:
    Vertex() : adj_(std::make_shared<std::vector<std::pair<int,int>>>()) {}
    Vertex(int id) : id_(id), adj_(std::make_shared<std::vector<std::pair<int,int>>>()) {}
    Vertex(int id, int bid, int sum, const std::shared_ptr<std::vector<std::pair<int,int>>>& adj) : id_(id), bid_(bid), sum_(sum), adj_(adj) {}

    int GetId() const { return id_; }
    int GetBid() const { return bid_; }
    int GetSum() const { return sum_; }
    const std::shared_ptr<std::vector<std::pair<int,int>>>& GetAdjList() const { return adj_; }

    void SetBid(int bid) { bid_ = bid; }


    double GetMemory() const {
        double ret = 3 * sizeof(int) + adj_->size() * sizeof(int);
        return ret;
    }
    bool operator<(const Vertex& other) const { return id_ < other.id_; }
    bool operator==(const Vertex& other) const { return id_ == other.id_; }

    friend void operator<<(axe::base::BinStream& bin_stream, const Vertex& v) { bin_stream << v.id_ << v.bid_ << v.sum_ << *(v.adj_); }
    friend void operator>>(axe::base::BinStream& bin_stream, Vertex& v) { bin_stream >> v.id_ >> v.bid_ >> v.sum_ >> *(v.adj_); }

    private:
    int id_,bid_,sum_;
    std::shared_ptr<std::vector<std::pair<int,int>>> adj_;
};

int ReadInt(const std::string& line, size_t& ptr) {
    int ret = 0;
    while (ptr < line.size() && !isdigit(line.at(ptr)))
        ++ptr;
    CHECK(ptr < line.size()) << "Invalid Input";
    while (ptr < line.size() && isdigit(line.at(ptr))) {
        ret = ret * 10 + line.at(ptr) - '0';
        ++ptr;
    }
    return ret;
}

DatasetPartition<Vertex> ParseLine(const std::string& line) {
    size_t ptr = 0;
    int id, k;
    auto adj = std::make_shared<std::vector<std::pair<int,int>>>();
    id = ReadInt(line, ptr);
    k = ReadInt(line, ptr);
    adj->reserve(k);
    for (int i = 0; i < k; ++i) {
        adj->push_back(std::make_pair(ReadInt(line, ptr), 1));

    }
    DatasetPartition<Vertex> ret;
    ret.push_back(Vertex(id, 0, k, adj));
    return ret;
}

class PageRank : public Job {
    public:
    void Run(TaskGraph* tg, const std::shared_ptr<Properties>& config) const override {
        auto input = config->GetOrSet("graph", "/datasets/graph/google-adj");
        int n_partitions = std::stoi(config->GetOrSet("parallelism", "20"));
        int n_iters = std::stoi(config->GetOrSet("n_iters", "5"));
        int block_size = std::stoi(config->GetOrSet("block_size", "20"));

        // Load data, partition by id, and sort within partition
        auto graph = HdfsSourceDataset(input, tg, n_partitions)
                    .Map([](const std::string& line) { return ParseLine(line); },
                        [](const std::vector<double>& input) {
                            double ret = 0;
                            for (double x : input) {
                                ret += x;
                            }
                            return ret * 2;
                        })
                    .PartitionBy([](const Vertex& v) { return v.GetBid(); }, n_partitions);
        
        graph.UpdatePartition([block_size](DatasetPartition<Vertex>& data) {
            std::sort(data.begin(), data.end(), [](const Vertex& a, const Vertex& b) { return a.GetId() < b.GetId(); });
            //
            for (int local_id = 0; local_id < data.size(); ++local_id) {
                int block_id = data.at(local_id / block_size).GetId();
                data.at(local_id).SetBid(block_id);
            }
        });

        graph.PartitionBy([](const Vertex& v) { return v.GetBid(); }, n_partitions);
        
        auto send_updates = [](const DatasetPartition<Vertex>& data, const DatasetPartition<std::pair<int, double>>& rank) {
            DatasetPartition<std::pair<int, double>> updates;
            int local_id = 0;
            for (const Vertex& v : data) {
                // simply ignore those with zero out degree
                while (local_id < rank.size() && rank.at(local_id).first < v.GetId()) {
                    ++local_id;
                }
                DCHECK_EQ(rank.at(local_id).first, v.GetId());
                double distribute = rank.at(local_id).second / v.GetSum() * 0.8;
                updates.push_back(std::make_pair(v.GetId(), 0.2));
                for (auto neighbor : *v.GetAdjList()) {
                    auto neighbor_block = std::lower_bound(data.begin(),data.end(),Vertex(neighbor.first))->GetBid();
                    if (neighbor_block == v.GetBid()){
                        updates.push_back(std::make_pair(neighbor.first, distribute * neighbor.second));
                    }
                }
                ++local_id;
            }
            return updates;
        };

        // initialize lpr
        auto rank_ptr = std::make_shared<axe::common::Dataset<std::pair<int, double>>>(graph.MapPartition([](const DatasetPartition<Vertex>& data) { //
            DatasetPartition<std::pair<int, double>> ret;
            ret.reserve(data.size());
            for (auto& v : data) {
                ret.push_back(std::make_pair(v.GetId(), 1.0));
            }
            return ret;
        }));

        for (int iter = 0; iter < n_iters; ++iter) {
            rank_ptr = std::make_shared<axe::common::Dataset<std::pair<int, double>>>( 
                graph.SharedDataMapPartitionWith(rank_ptr.get(), send_updates) //
                    .ReduceBy([](const std::pair<int, double>& id_rank) { return id_rank.first; },
                                [](std::pair<int, double>& agg, const std::pair<int, double>& update) { agg.second += update.second; }, n_partitions));
        }

        auto block_edges = std::make_shared<axe::common::Dataset<std::pair<int, int>>>(graph.MapPartition([](const DatasetPartition<Vertex>& data) { //
            DatasetPartition<std::pair<int, int>> ret;
            size_t reserve_size = 0;
            for (auto& v : data) {
                reserve_size += v.GetAdjList()->size();
            }
            ret.reserve(reserve_size);
            for (auto& v : data) {
                for (auto neighbor : *v.GetAdjList()) {
                    if(v.GetBid() != neighbor.first)
                        ret.push_back(std::make_pair(v.GetBid(), neighbor.first));
                }
            }
            return ret;
        })
        .PartitionBy([](const std::pair<int, int>& v) { return v.second; }, n_partitions));

        block_edges->UpdatePartition([](DatasetPartition<std::pair<int, int>>& data){
            for (auto &v : data){
                std::swap(v.first, v.second);
            }
            std::sort(data.begin(), data.end());
        });

        block_edges = std::make_shared<axe::common::Dataset<std::pair<int, int>>>(
            block_edges->MapPartitionWith(&graph, [](DatasetPartition<std::pair<int, int>>& data, const DatasetPartition<Vertex>& graph) {
                //
                DatasetPartition<std::pair<int, int>> ret;
                int local_id = 0;
                for (auto &v : data){
                    while (local_id < graph.size() && graph.at(local_id).GetId() < v.first)
                        ++local_id;
                    if (local_id >= graph.size())
                        break;
                    
                    if (graph.at(local_id).GetId() == v.first){
                        if (graph.at(local_id).GetBid() != v.second)
                            ret.push_back(std::make_pair(v.second, graph.at(local_id).GetBid()));
                    }

                }
                return ret;
        })
        .PartitionBy([](const std::pair<int, int>& v) { return v.first; }, n_partitions));

        block_edges->UpdatePartition([](DatasetPartition<std::pair<int, int>>& data){
            std::sort(data.begin(), data.end());
        });

        // initialize block graph
        auto bgraph = axe::common::Dataset<Vertex>(block_edges->MapPartition([](const DatasetPartition<std::pair<int, int>>& data) { //
            DatasetPartition<Vertex> ret;
            Vertex block_v(0);
            int last_bid = -1;
            int current_block = -1;
            int current_count = 0;
            for (auto& v : data) {
                if (v.first != last_bid){
                    if (current_count > 0){
                        block_v.GetAdjList()->push_back(std::make_pair(current_block, current_count));
                        current_count = 0;
                    }
                    ret.push_back(block_v);
                    block_v = Vertex(v.first);
                    current_block = v.second;
                    current_count = 0;
                }
                else if (v.second != current_block){
                    if (current_count > 0){
                        block_v.GetAdjList()->push_back(std::make_pair(current_block, current_count));
                        current_count = 0;
                    }
                }
                ++current_count;
                last_bid = v.first;
            }
            if(last_bid != -1){
                if (current_count > 0){
                    block_v.GetAdjList()->push_back(std::make_pair(current_block, current_count));
                }
                ret.push_back(block_v);
            }

            return ret;
        }));

        // initialize br
        auto br_ptr = std::make_shared<axe::common::Dataset<std::pair<int, double>>>(bgraph.MapPartition([](const DatasetPartition<Vertex>& data) { //11
            DatasetPartition<std::pair<int, double>> ret;
            ret.reserve(data.size());
            for (auto& v : data) {
                ret.push_back(std::make_pair(v.GetId(), 1.0));
            }
            return ret;
        }));

        for (int iter = 0; iter < n_iters; ++iter) {
            br_ptr = std::make_shared<axe::common::Dataset<std::pair<int, double>>>(      
                bgraph.SharedDataMapPartitionWith(br_ptr.get(), send_updates)
                    .ReduceBy([](const std::pair<int, double>& id_rank) { return id_rank.first; }, //12
                                [](std::pair<int, double>& agg, const std::pair<int, double>& update) { agg.second += update.second; }, n_partitions));
        }

        br_ptr = std::make_shared<axe::common::Dataset<std::pair<int, double>>>(
            graph.SharedDataMapPartitionWith(br_ptr.get(), [](const DatasetPartition<Vertex>& data, const DatasetPartition<std::pair<int, double>>& br) { //16
                DatasetPartition<std::pair<int, double>> ret;
                ret.reserve(data.size());
                int local_id = 0;
                for (const Vertex& v : data) {
                    while (local_id < br.size() && v.GetBid() > br.at(local_id).first){
                        ++local_id;
                    }
                    if (v.GetBid() == br.at(local_id).first){
                        ret.push_back(std::make_pair(v.GetId(), br.at(local_id).second));
                    }
                    else{
                        ret.push_back(std::make_pair(v.GetId(), 1.0));
                    }
                }
                return ret;
        }));

        rank_ptr = std::make_shared<axe::common::Dataset<std::pair<int, double>>>(
            rank_ptr->MapPartitionWith(br_ptr.get(), [](const DatasetPartition<std::pair<int, double>>& data, const DatasetPartition<std::pair<int, double>>& lpr) { //17
                DatasetPartition<std::pair<int, double>> ret;
                ret.reserve(data.size());
                int local_id = 0;
                for (auto& v : data) {
                    ret.push_back(std::make_pair(v.first, v.second * lpr.at(local_id).second));
                    ++local_id;
                }
                return ret;
        }));

        graph.UpdatePartition([](DatasetPartition<Vertex>& data) { //18
            // reset block id
            for (int local_id = 0; local_id < data.size(); ++local_id) { 
                data.at(local_id).SetBid(0);
            }
        });

        // main loop
        for (int iter = 0; iter < n_iters; ++iter) {
            rank_ptr = std::make_shared<axe::common::Dataset<std::pair<int, double>>>(
                graph.SharedDataMapPartitionWith(rank_ptr.get(), send_updates) //19
                    .ReduceBy([](const std::pair<int, double>& id_rank) { return id_rank.first; },
                                [](std::pair<int, double>& agg, const std::pair<int, double>& update) { agg.second += update.second; }, n_partitions));
        }

        // select top 10
        auto select_top10 = [](const DatasetPartition<std::pair<int, double>>& data) {
            DatasetPartition<std::pair<int, double>> ret;
            auto greater = [&data](int a, int b) { return data.at(a).second > data.at(b).second; };
            std::priority_queue<int, std::vector<int>, decltype(greater)> top10(greater);
            int idx = 0;
            for (auto& rank : data) {
                if (top10.size() < 10) {
                    top10.push(idx);
                } 
                else if (rank.second > data.at(top10.top()).second) {
                    top10.pop();
                    top10.push(idx);
                }
                ++idx;
            }
            while (!top10.empty()) {
                ret.push_back(data.at(top10.top()));
                top10.pop();
            }
            return ret;
        };

        rank_ptr->MapPartition(select_top10)
            .PartitionBy([](const std::pair<int, double>&) { return 0; }, 1)
            .MapPartition(select_top10)
            .ApplyRead([](const DatasetPartition<std::pair<int, double>>& data) {
                for (auto& rank : data) {
                    LOG(INFO) << "id: " << rank.first << ", rank: " << rank.second;
                }
                google::FlushLogFiles(google::INFO);
            });
        axe::common::JobDriver::ReversePrintTaskGraph(*tg);
    }
};

int main(int argc, char** argv) {
  axe::common::JobDriver::Run(argc, argv, PageRank());
  return 0;
}
