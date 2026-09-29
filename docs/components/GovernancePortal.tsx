import { useEffect, useState } from "react";
import StatusBadge from "@/components/ui/StatusBadge";

interface Proposal {
  id: string;
  title: string;
  description: string;
  proposer: string;
  status: "pending" | "voting" | "approved" | "rejected";
  yesVotes: number;
  noVotes: number;
  totalVotes: number;
  yesPercentage: number;
  noPercentage: number;
}

const mockProposals: Proposal[] = [
  {
    id: "p1",
    title: "Protocol Upgrade",
    description: "Upgrade the protocol",
    proposer: "Admin",
    status: "voting",
    yesVotes: 47,
    noVotes: 13,
    totalVotes: 60,
    yesPercentage: 78.3,
    noPercentage: 21.7,
  },
  {
    id: "p2",
    title: "Fee Review",
    description: "Review fees",
    proposer: "Finance",
    status: "approved",
    yesVotes: 55,
    noVotes: 5,
    totalVotes: 60,
    yesPercentage: 91.7,
    noPercentage: 8.3,
  },
];

export function GovernancePortal() {
  const [proposals] = useState<Proposal[]>(mockProposals);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    const timer = setTimeout(() => setLoading(false), 500);

    return () => clearTimeout(timer);
  }, []);

  if (loading) {
    return <div>Loading...</div>;
  }

  return (
    <div>
      <h2>Governance Portal</h2>

      {proposals.map((proposal) => (
        <div
          key={proposal.id}
          style={{
            border: "1px solid #ccc",
            padding: "10px",
            margin: "10px 0",
          }}
        >
          <h3>{proposal.title}</h3>
          <p>{proposal.description}</p>
          <StatusBadge status={proposal.status} />

          <div>
            {proposal.yesPercentage.toFixed(1)}% yes |{" "}
            {proposal.noPercentage.toFixed(1)}% no
          </div>
        </div>
      ))}
    </div>
  );
}
